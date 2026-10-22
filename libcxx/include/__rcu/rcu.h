// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___RCU_RCU_H
#define _LIBCPP___RCU_RCU_H

#include <__condition_variable/condition_variable.h>
#include <__config>
#include <__cstddef/size_t.h>
#include <__memory/unique_ptr.h>
#include <__mutex/lock_guard.h>
#include <__mutex/mutex.h>
#include <__mutex/unique_lock.h>
#include <__utility/exchange.h>
#include <__utility/move.h>
#include <__utility/pair.h>
#include <__vector/vector.h>
#include <cstdint>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

namespace __rcu {

struct __retired_node {
  __retired_node* __next_ = nullptr;
  void* __object_ = nullptr;
  void (*__evaluate_)(__retired_node*) noexcept = nullptr;
  // The epoch of the domain when the evaluation was scheduled: the regions of RCU protection that started before
  // it (their epoch is not greater) have to end before the evaluation, the ones that started after it do not.
  uint64_t __epoch_ = 0;
};

// The epochs of the regions of RCU protection a thread holds, innermost last (an unlock closes the innermost
// region of its thread, [saferecl.rcu.domain.mem]).
inline _LIBCPP_HIDE_FROM_ABI vector<pair<const void*, uint64_t>>& __thread_regions() noexcept {
  static thread_local vector<pair<const void*, uint64_t>> __regions;
  return __regions;
}

} // namespace __rcu

class rcu_domain;
_LIBCPP_HIDE_FROM_ABI rcu_domain& rcu_default_domain() noexcept;
template <class _Tp, class _Dp = default_delete<_Tp>>
_LIBCPP_HIDE_FROM_ABI void rcu_retire(_Tp*, _Dp = _Dp(), rcu_domain& = rcu_default_domain());

class rcu_domain {
  mutex __mutex_;
  condition_variable __cv_;
  size_t __readers_ = 0;
  bool __synchronizing_ = false;
  __rcu::__retired_node* __retired_ = nullptr;
  uint64_t __epoch_ = 0;
  // (epoch, number of open regions that started in it), by increasing epoch
  vector<pair<uint64_t, size_t>> __active_;

  _LIBCPP_HIDE_FROM_ABI bool __regions_ended_up_to(uint64_t __epoch) const noexcept {
    return __active_.empty() || __active_.front().first > __epoch;
  }

  _LIBCPP_HIDE_FROM_ABI void __retire(__rcu::__retired_node* __node) noexcept {
    lock_guard<mutex> __lock(__mutex_);
    __node->__epoch_ = __epoch_++;
    __node->__next_ = __retired_;
    __retired_ = __node;
  }

  // [saferecl.rcu.domain]: there is no default constructor; the default domain is made here.
  _LIBCPP_HIDE_FROM_ABI rcu_domain() = default;
  _LIBCPP_HIDE_FROM_ABI friend rcu_domain& rcu_default_domain() noexcept;
  _LIBCPP_HIDE_FROM_ABI friend void rcu_synchronize(rcu_domain&) noexcept;
  _LIBCPP_HIDE_FROM_ABI friend void rcu_barrier(rcu_domain&) noexcept;
  template <class _Tp, class _Dp>
  friend void rcu_retire(_Tp*, _Dp, rcu_domain&);
  template <class _Tp, class _Dp>
  friend class rcu_obj_base;

public:
  rcu_domain(const rcu_domain&) = delete;
  rcu_domain& operator=(const rcu_domain&) = delete;

  _LIBCPP_HIDE_FROM_ABI void lock() noexcept {
    unique_lock<mutex> __lock(__mutex_);
    __cv_.wait(__lock, [this] { return !__synchronizing_; });
    ++__readers_;
    if (!__active_.empty() && __active_.back().first == __epoch_)
      ++__active_.back().second;
    else
      __active_.emplace_back(__epoch_, 1);
    __rcu::__thread_regions().emplace_back(this, __epoch_);
  }

  _LIBCPP_HIDE_FROM_ABI bool try_lock() noexcept {
    lock();
    return true;
  }

  _LIBCPP_HIDE_FROM_ABI void unlock() noexcept {
    lock_guard<mutex> __lock(__mutex_);
    // close the innermost region of this thread on this domain
    uint64_t __epoch = __epoch_;
    auto& __regions  = __rcu::__thread_regions();
    for (size_t __i = __regions.size(); __i != 0; --__i) {
      if (__regions[__i - 1].first == this) {
        __epoch = __regions[__i - 1].second;
        __regions.erase(__regions.begin() + (__i - 1));
        break;
      }
    }
    for (auto __it = __active_.begin(); __it != __active_.end(); ++__it) {
      if (__it->first == __epoch) {
        if (--__it->second == 0)
          __active_.erase(__it);
        break;
      }
    }
    --__readers_;
    __cv_.notify_all();
  }
};

_LIBCPP_HIDE_FROM_ABI inline rcu_domain& rcu_default_domain() noexcept {
  static rcu_domain __domain;
  return __domain;
}

_LIBCPP_HIDE_FROM_ABI inline void rcu_synchronize(rcu_domain& __dom = rcu_default_domain()) noexcept {
  unique_lock<mutex> __lock(__dom.__mutex_);
  __dom.__synchronizing_ = true;
  __dom.__cv_.wait(__lock, [&__dom] { return __dom.__readers_ == 0; });
  __dom.__synchronizing_ = false;
  __lock.unlock();
  __dom.__cv_.notify_all();
}

// [saferecl.rcu.domain.func]: evaluates the evaluations scheduled before the call. An evaluation waits for the end of
// the regions of RCU protection that started before it was scheduled (not for the ones that started later: the
// calling thread may hold one), and rcu_barrier does not imply rcu_synchronize.
_LIBCPP_HIDE_FROM_ABI inline void rcu_barrier(rcu_domain& __dom = rcu_default_domain()) noexcept {
  __rcu::__retired_node* __node;
  uint64_t __last_epoch = 0;
  {
    unique_lock<mutex> __lock(__dom.__mutex_);
    __node = std::exchange(__dom.__retired_, nullptr);
    for (auto* __n = __node; __n != nullptr; __n = __n->__next_)
      if (__n->__epoch_ > __last_epoch)
        __last_epoch = __n->__epoch_;
    if (__node != nullptr)
      __dom.__cv_.wait(__lock, [&] { return __dom.__regions_ended_up_to(__last_epoch); });
  }
  while (__node != nullptr) {
    __rcu::__retired_node* __next = __node->__next_;
    __node->__evaluate_(__node);
    __node = __next;
  }
}

template <class _Tp, class _Dp = default_delete<_Tp>>
class rcu_obj_base : private __rcu::__retired_node {
  _Dp __deleter_;

  _LIBCPP_HIDE_FROM_ABI static void __evaluate(__rcu::__retired_node* __node) noexcept {
    auto* __base = static_cast<rcu_obj_base*>(__node);
    __base->__deleter_(static_cast<_Tp*>(__base->__object_));
  }

public:
  _LIBCPP_HIDE_FROM_ABI void retire(_Dp __deleter = _Dp(), rcu_domain& __dom = rcu_default_domain()) noexcept {
    __deleter_ = std::move(__deleter);
    this->__object_ = static_cast<_Tp*>(this);
    this->__evaluate_ = &rcu_obj_base::__evaluate;
    __dom.__retire(this);
  }

protected:
  _LIBCPP_HIDE_FROM_ABI rcu_obj_base()                               = default;
  _LIBCPP_HIDE_FROM_ABI rcu_obj_base(const rcu_obj_base&)            = default;
  _LIBCPP_HIDE_FROM_ABI rcu_obj_base(rcu_obj_base&&)                 = default;
  _LIBCPP_HIDE_FROM_ABI rcu_obj_base& operator=(const rcu_obj_base&) = default;
  _LIBCPP_HIDE_FROM_ABI rcu_obj_base& operator=(rcu_obj_base&&)      = default;
  _LIBCPP_HIDE_FROM_ABI ~rcu_obj_base()                              = default;
};

template <class _Tp, class _Dp>
struct __rcu_retired_node : __rcu::__retired_node {
  _Tp* __ptr_;
  _Dp __deleter_;

  _LIBCPP_HIDE_FROM_ABI __rcu_retired_node(_Tp* __ptr, _Dp&& __deleter)
      : __ptr_(__ptr), __deleter_(std::move(__deleter)) {
    this->__object_ = __ptr;
    this->__evaluate_ = &__rcu_retired_node::__evaluate;
  }

  _LIBCPP_HIDE_FROM_ABI static void __evaluate(__rcu::__retired_node* __node) noexcept {
    auto* __self = static_cast<__rcu_retired_node*>(__node);
    __self->__deleter_(__self->__ptr_);
    delete __self;
  }
};

template <class _Tp, class _Dp>
_LIBCPP_HIDE_FROM_ABI void rcu_retire(_Tp* __ptr, _Dp __deleter, rcu_domain& __dom) {
  __dom.__retire(new __rcu_retired_node<_Tp, _Dp>(__ptr, std::move(__deleter)));
}

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP___RCU_RCU_H
