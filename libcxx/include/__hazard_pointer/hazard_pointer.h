// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___HAZARD_POINTER_HAZARD_POINTER_H
#define _LIBCPP___HAZARD_POINTER_HAZARD_POINTER_H

#include <__atomic/atomic.h>
#include <__config>
#include <__cstddef/nullptr_t.h>
#include <__memory/addressof.h>
#include <__memory/unique_ptr.h>
#include <__mutex/lock_guard.h>
#include <__mutex/mutex.h>
#include <__utility/exchange.h>
#include <__utility/move.h>
#include <__utility/swap.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

namespace __hazard_pointer {

struct __retired_node {
  __retired_node* __next_ = nullptr;
  const void* __object_          = nullptr;
  void (*__reclaim_)(__retired_node*) noexcept = nullptr;
};

struct __record {
  atomic<const void*> __protected_{nullptr};
  __record* __next_ = nullptr;
  bool __in_use_ = false;
};

class __domain {
  mutex __mutex_;
  __record* __records_ = nullptr;
  __retired_node* __retired_ = nullptr;

  __retired_node* __collect_locked() noexcept {
    __retired_node* __reclaim = nullptr;
    __retired_node** __current = &__retired_;
    while (*__current != nullptr) {
      __retired_node* __node = *__current;
      bool __protected = false;
      for (__record* __record = __records_; __record != nullptr; __record = __record->__next_) {
        if (__record->__protected_.load(memory_order_acquire) == __node->__object_) {
          __protected = true;
          break;
        }
      }
      if (__protected) {
        __current = &__node->__next_;
      } else {
        *__current = __node->__next_;
        __node->__next_ = __reclaim;
        __reclaim = __node;
      }
    }
    return __reclaim;
  }

public:
  __record* __acquire_record() {
    lock_guard<mutex> __lock(__mutex_);
    for (__record* __record = __records_; __record != nullptr; __record = __record->__next_) {
      if (!__record->__in_use_) {
        __record->__in_use_ = true;
        return __record;
      }
    }
    __record* __new_record = new __record;
    __new_record->__in_use_ = true;
    __new_record->__next_ = __records_;
    __records_ = __new_record;
    return __new_record;
  }

  void __release_record(__record* __record) noexcept {
    __record->__protected_.store(nullptr, memory_order_release);
    lock_guard<mutex> __lock(__mutex_);
    __record->__in_use_ = false;
  }

  void __retire(__retired_node* __node) noexcept {
    __retired_node* __reclaim;
    {
      lock_guard<mutex> __lock(__mutex_);
      __node->__next_ = __retired_;
      __retired_ = __node;
      __reclaim = __collect_locked();
    }
    while (__reclaim != nullptr) {
      __retired_node* __next = __reclaim->__next_;
      __reclaim->__reclaim_(__reclaim);
      __reclaim = __next;
    }
  }
};

inline __domain& __get_domain() {
  static __domain __domain;
  return __domain;
}

} // namespace __hazard_pointer

template <class _Tp, class _Dp = default_delete<_Tp>>
class hazard_pointer_obj_base : private __hazard_pointer::__retired_node {
  _Dp __deleter_;

  static void __reclaim(__hazard_pointer::__retired_node* __node) noexcept {
    auto* __base = static_cast<hazard_pointer_obj_base*>(__node);
    __base->__deleter_(static_cast<_Tp*>(__base));
  }

public:
  void retire(_Dp __deleter = _Dp()) noexcept {
    __deleter_ = std::move(__deleter);
    this->__object_  = static_cast<_Tp*>(this);
    this->__reclaim_ = &hazard_pointer_obj_base::__reclaim;
    __hazard_pointer::__get_domain().__retire(this);
  }

protected:
  hazard_pointer_obj_base() = default;
  hazard_pointer_obj_base(const hazard_pointer_obj_base&) = default;
  hazard_pointer_obj_base(hazard_pointer_obj_base&&) = default;
  hazard_pointer_obj_base& operator=(const hazard_pointer_obj_base&) = default;
  hazard_pointer_obj_base& operator=(hazard_pointer_obj_base&&) = default;
  ~hazard_pointer_obj_base() = default;
};

class hazard_pointer {
  __hazard_pointer::__record* __record_ = nullptr;

  explicit hazard_pointer(__hazard_pointer::__record* __record) noexcept : __record_(__record) {}
  friend hazard_pointer make_hazard_pointer();

public:
  hazard_pointer() noexcept = default;
  hazard_pointer(hazard_pointer&& __other) noexcept : __record_(std::exchange(__other.__record_, nullptr)) {}
  hazard_pointer& operator=(hazard_pointer&& __other) noexcept {
    if (this != std::addressof(__other)) {
      if (__record_ != nullptr)
        __hazard_pointer::__get_domain().__release_record(__record_);
      __record_ = std::exchange(__other.__record_, nullptr);
    }
    return *this;
  }
  ~hazard_pointer() {
    if (__record_ != nullptr)
      __hazard_pointer::__get_domain().__release_record(__record_);
  }

  [[nodiscard]] bool empty() const noexcept { return __record_ == nullptr; }

  template <class _Tp>
  _Tp* protect(const atomic<_Tp*>& __src) noexcept {
    _Tp* __ptr = __src.load(memory_order_relaxed);
    while (!try_protect(__ptr, __src)) {}
    return __ptr;
  }

  template <class _Tp>
  bool try_protect(_Tp*& __ptr, const atomic<_Tp*>& __src) noexcept {
    _Tp* __old = __ptr;
    reset_protection(__old);
    __ptr = __src.load(memory_order_acquire);
    if (__old != __ptr)
      reset_protection();
    return __old == __ptr;
  }

  template <class _Tp>
  void reset_protection(const _Tp* __ptr) noexcept {
    if (__ptr == nullptr)
      reset_protection();
    else
      __record_->__protected_.store(__ptr, memory_order_release);
  }

  void reset_protection(nullptr_t = nullptr) noexcept { __record_->__protected_.store(nullptr, memory_order_release); }

  void swap(hazard_pointer& __other) noexcept { std::swap(__record_, __other.__record_); }
};

inline hazard_pointer make_hazard_pointer() { return hazard_pointer(__hazard_pointer::__get_domain().__acquire_record()); }

inline void swap(hazard_pointer& __a, hazard_pointer& __b) noexcept { __a.swap(__b); }

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP___HAZARD_POINTER_HAZARD_POINTER_H
