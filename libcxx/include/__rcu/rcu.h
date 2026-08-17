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
#include <__utility/move.h>
#include <__utility/exchange.h>

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
};

} // namespace __rcu

class rcu_domain;
rcu_domain& rcu_default_domain() noexcept;
template <class _Tp, class _Dp = default_delete<_Tp>>
void rcu_retire(_Tp*, _Dp = _Dp(), rcu_domain& = rcu_default_domain());

class rcu_domain {
  mutex __mutex_;
  condition_variable __cv_;
  size_t __readers_ = 0;
  bool __synchronizing_ = false;
  __rcu::__retired_node* __retired_ = nullptr;

  __rcu::__retired_node* __take_retired() noexcept {
    lock_guard<mutex> __lock(__mutex_);
    return std::exchange(__retired_, nullptr);
  }

  void __retire(__rcu::__retired_node* __node) noexcept {
    lock_guard<mutex> __lock(__mutex_);
    __node->__next_ = __retired_;
    __retired_ = __node;
  }

  friend void rcu_synchronize(rcu_domain&) noexcept;
  friend void rcu_barrier(rcu_domain&) noexcept;
  template <class _Tp, class _Dp>
  friend void rcu_retire(_Tp*, _Dp, rcu_domain&);
  template <class _Tp, class _Dp>
  friend class rcu_obj_base;

public:
  rcu_domain() = default;
  rcu_domain(const rcu_domain&) = delete;
  rcu_domain& operator=(const rcu_domain&) = delete;

  void lock() noexcept {
    unique_lock<mutex> __lock(__mutex_);
    __cv_.wait(__lock, [this] { return !__synchronizing_; });
    ++__readers_;
  }

  bool try_lock() noexcept {
    lock();
    return true;
  }

  void unlock() noexcept {
    lock_guard<mutex> __lock(__mutex_);
    --__readers_;
    if (__readers_ == 0)
      __cv_.notify_all();
  }
};

inline rcu_domain& rcu_default_domain() noexcept {
  static rcu_domain __domain;
  return __domain;
}

inline void rcu_synchronize(rcu_domain& __dom = rcu_default_domain()) noexcept {
  unique_lock<mutex> __lock(__dom.__mutex_);
  __dom.__synchronizing_ = true;
  __dom.__cv_.wait(__lock, [&__dom] { return __dom.__readers_ == 0; });
  __dom.__synchronizing_ = false;
  __lock.unlock();
  __dom.__cv_.notify_all();
}

inline void rcu_barrier(rcu_domain& __dom = rcu_default_domain()) noexcept {
  rcu_synchronize(__dom);
  while (__rcu::__retired_node* __node = __dom.__take_retired()) {
    while (__node != nullptr) {
      __rcu::__retired_node* __next = __node->__next_;
      __node->__evaluate_(__node);
      __node = __next;
    }
  }
}

template <class _Tp, class _Dp = default_delete<_Tp>>
class rcu_obj_base : private __rcu::__retired_node {
  _Dp __deleter_;

  static void __evaluate(__rcu::__retired_node* __node) noexcept {
    auto* __base = static_cast<rcu_obj_base*>(__node);
    __base->__deleter_(static_cast<_Tp*>(__base->__object_));
  }

public:
  void retire(_Dp __deleter = _Dp(), rcu_domain& __dom = rcu_default_domain()) noexcept {
    __deleter_ = std::move(__deleter);
    this->__object_ = static_cast<_Tp*>(this);
    this->__evaluate_ = &rcu_obj_base::__evaluate;
    __dom.__retire(this);
  }

protected:
  rcu_obj_base() = default;
  rcu_obj_base(const rcu_obj_base&) = default;
  rcu_obj_base(rcu_obj_base&&) = default;
  rcu_obj_base& operator=(const rcu_obj_base&) = default;
  rcu_obj_base& operator=(rcu_obj_base&&) = default;
  ~rcu_obj_base() = default;
};

template <class _Tp, class _Dp>
struct __rcu_retired_node : __rcu::__retired_node {
  _Tp* __ptr_;
  _Dp __deleter_;

  __rcu_retired_node(_Tp* __ptr, _Dp&& __deleter) : __ptr_(__ptr), __deleter_(std::move(__deleter)) {
    this->__object_ = __ptr;
    this->__evaluate_ = &__rcu_retired_node::__evaluate;
  }

  static void __evaluate(__rcu::__retired_node* __node) noexcept {
    auto* __self = static_cast<__rcu_retired_node*>(__node);
    __self->__deleter_(__self->__ptr_);
    delete __self;
  }
};

template <class _Tp, class _Dp>
void rcu_retire(_Tp* __ptr, _Dp __deleter, rcu_domain& __dom) {
  __dom.__retire(new __rcu_retired_node<_Tp, _Dp>(__ptr, std::move(__deleter)));
}

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP___RCU_RCU_H
