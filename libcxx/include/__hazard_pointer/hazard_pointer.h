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
#include <__type_traits/integral_constant.h>
#include <__type_traits/is_base_of.h>
#include <__type_traits/is_class.h>
#include <__utility/declval.h>
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

  _LIBCPP_HIDE_FROM_ABI __retired_node* __collect_locked() noexcept {
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
  _LIBCPP_HIDE_FROM_ABI __record* __acquire_record() {
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

  _LIBCPP_HIDE_FROM_ABI void __release_record(__record* __record) noexcept {
    __record->__protected_.store(nullptr, memory_order_release);
    lock_guard<mutex> __lock(__mutex_);
    __record->__in_use_ = false;
  }

  _LIBCPP_HIDE_FROM_ABI void __retire(__retired_node* __node) noexcept {
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

_LIBCPP_HIDE_FROM_ABI inline __domain& __get_domain() {
  static __domain __domain;
  return __domain;
}

} // namespace __hazard_pointer

template <class _Tp, class _Dp>
class hazard_pointer_obj_base;

// [saferecl.hp.general]: a class type T is hazard-protectable if it has exactly one base class of type
// hazard_pointer_obj_base<T, D> for some D, that base is public and non-virtual, and it has no base classes of
// type hazard_pointer_obj_base<T2, D2> for any other combination T2, D2.
template <class _Tp, class _Dp>
_Dp __hazard_pointer_deleter_of(const hazard_pointer_obj_base<_Tp, _Dp>*);

template <class _Tp>
struct __is_hazard_protectable : false_type {};

template <class _Tp>
  requires requires(_Tp* __p) { std::__hazard_pointer_deleter_of<_Tp>(__p); }
struct __is_hazard_protectable<_Tp> {
  using __deleter _LIBCPP_NODEBUG = decltype(std::__hazard_pointer_deleter_of<_Tp>(std::declval<_Tp*>()));
  // Deducing the deleter requires the base to be public; the base must also be non-virtual.
  // The hidden-friend probe rejects multiple hazard_pointer_obj_base bases, including ones for other T.
  static constexpr bool value =
      is_class_v<_Tp> && !is_virtual_base_of_v<hazard_pointer_obj_base<_Tp, __deleter>, _Tp> &&
      // NOLINTNEXTLINE(libcpp-robust-against-adl) intentional: hidden-friend probe
      requires(_Tp* __p) { __hazard_pointer_base_probe(__p); };
};

template <class _Tp, class _Dp = default_delete<_Tp>>
class hazard_pointer_obj_base : private __hazard_pointer::__retired_node {
  friend void __hazard_pointer_base_probe(const hazard_pointer_obj_base*);

  template <class>
  friend struct __is_hazard_protectable;

  _Dp __deleter_;

  _LIBCPP_HIDE_FROM_ABI static void __reclaim(__hazard_pointer::__retired_node* __node) noexcept {
    auto* __base = static_cast<hazard_pointer_obj_base*>(__node);
    __base->__deleter_(static_cast<_Tp*>(__base));
  }

public:
  _LIBCPP_HIDE_FROM_ABI void retire(_Dp __deleter = _Dp()) noexcept {
    static_assert(__is_hazard_protectable<_Tp>::value, "T must be a hazard-protectable type");
    __deleter_ = std::move(__deleter);
    this->__object_  = static_cast<_Tp*>(this);
    this->__reclaim_ = &hazard_pointer_obj_base::__reclaim;
    __hazard_pointer::__get_domain().__retire(this);
  }

protected:
  _LIBCPP_HIDE_FROM_ABI hazard_pointer_obj_base()                                          = default;
  _LIBCPP_HIDE_FROM_ABI hazard_pointer_obj_base(const hazard_pointer_obj_base&)            = default;
  _LIBCPP_HIDE_FROM_ABI hazard_pointer_obj_base(hazard_pointer_obj_base&&)                 = default;
  _LIBCPP_HIDE_FROM_ABI hazard_pointer_obj_base& operator=(const hazard_pointer_obj_base&) = default;
  _LIBCPP_HIDE_FROM_ABI hazard_pointer_obj_base& operator=(hazard_pointer_obj_base&&)      = default;
  _LIBCPP_HIDE_FROM_ABI ~hazard_pointer_obj_base()                                         = default;
};

class hazard_pointer {
  __hazard_pointer::__record* __record_ = nullptr;

  _LIBCPP_HIDE_FROM_ABI explicit hazard_pointer(__hazard_pointer::__record* __record) noexcept : __record_(__record) {}
  _LIBCPP_HIDE_FROM_ABI friend hazard_pointer make_hazard_pointer();

public:
  _LIBCPP_HIDE_FROM_ABI hazard_pointer() noexcept = default;
  _LIBCPP_HIDE_FROM_ABI hazard_pointer(hazard_pointer&& __other) noexcept
      : __record_(std::exchange(__other.__record_, nullptr)) {}
  _LIBCPP_HIDE_FROM_ABI hazard_pointer& operator=(hazard_pointer&& __other) noexcept {
    if (this != std::addressof(__other)) {
      if (__record_ != nullptr)
        __hazard_pointer::__get_domain().__release_record(__record_);
      __record_ = std::exchange(__other.__record_, nullptr);
    }
    return *this;
  }
  _LIBCPP_HIDE_FROM_ABI ~hazard_pointer() {
    if (__record_ != nullptr)
      __hazard_pointer::__get_domain().__release_record(__record_);
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI bool empty() const noexcept { return __record_ == nullptr; }

  template <class _Tp>
  _LIBCPP_HIDE_FROM_ABI _Tp* protect(const atomic<_Tp*>& __src) noexcept {
    _Tp* __ptr = __src.load(memory_order_relaxed);
    while (!try_protect(__ptr, __src)) {}
    return __ptr;
  }

  template <class _Tp>
  _LIBCPP_HIDE_FROM_ABI bool try_protect(_Tp*& __ptr, const atomic<_Tp*>& __src) noexcept {
    static_assert(__is_hazard_protectable<_Tp>::value, "T must be a hazard-protectable type");
    _Tp* __old = __ptr;
    reset_protection(__old);
    __ptr = __src.load(memory_order_acquire);
    if (__old != __ptr)
      reset_protection();
    return __old == __ptr;
  }

  template <class _Tp>
  _LIBCPP_HIDE_FROM_ABI void reset_protection(const _Tp* __ptr) noexcept {
    static_assert(__is_hazard_protectable<_Tp>::value, "T must be a hazard-protectable type");
    if (__ptr == nullptr)
      reset_protection();
    else
      __record_->__protected_.store(__ptr, memory_order_release);
  }

  _LIBCPP_HIDE_FROM_ABI void reset_protection(nullptr_t = nullptr) noexcept {
    __record_->__protected_.store(nullptr, memory_order_release);
  }

  _LIBCPP_HIDE_FROM_ABI void swap(hazard_pointer& __other) noexcept { std::swap(__record_, __other.__record_); }
};

_LIBCPP_HIDE_FROM_ABI inline hazard_pointer make_hazard_pointer() {
  return hazard_pointer(__hazard_pointer::__get_domain().__acquire_record());
}

_LIBCPP_HIDE_FROM_ABI inline void swap(hazard_pointer& __a, hazard_pointer& __b) noexcept { __a.swap(__b); }

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP___HAZARD_POINTER_HAZARD_POINTER_H
