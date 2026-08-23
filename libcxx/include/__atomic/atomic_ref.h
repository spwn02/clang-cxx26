// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//                        Kokkos v. 4.0
//       Copyright (2022) National Technology & Engineering
//               Solutions of Sandia, LLC (NTESS).
//
// Under the terms of Contract DE-NA0003525 with NTESS,
// the U.S. Government retains certain rights in this software.
//
//===---------------------------------------------------------------------===//

#ifndef _LIBCPP___ATOMIC_ATOMIC_REF_H
#define _LIBCPP___ATOMIC_ATOMIC_REF_H

#include <__assert>
#include <__atomic/atomic_sync.h>
#include <__atomic/check_memory_order.h>
#include <__atomic/memory_order.h>
#include <__atomic/to_gcc_order.h>
#include <__concepts/arithmetic.h>
#include <__concepts/same_as.h>
#include <__config>
#include <__cstddef/byte.h>
#include <__cstddef/ptrdiff_t.h>
#include <__memory/addressof.h>
#include <__type_traits/has_unique_object_representation.h>
#include <__type_traits/is_const.h>
#include <__type_traits/is_constant_evaluated.h>
#include <__type_traits/is_pointer.h>
#include <__type_traits/is_scalar.h>
#include <__type_traits/is_trivially_copyable.h>
#include <__type_traits/is_volatile.h>
#include <__type_traits/remove_cv.h>
#include <__type_traits/remove_pointer.h>
#include <cstdint>
#include <cstring>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 20

// These types are required to make __atomic_is_always_lock_free work across GCC and Clang.
// The purpose of this trick is to make sure that we provide an object with the correct alignment
// to __atomic_is_always_lock_free, since that answer depends on the alignment.
template <size_t _Alignment>
struct __alignment_checker_type {
  alignas(_Alignment) char __data;
};

template <size_t _Alignment>
struct __get_aligner_instance {
  static constexpr __alignment_checker_type<_Alignment> __instance{};
};

template <class _Tp>
struct __atomic_ref_base {
public:
  // P3323R1: atomic_ref<T> supports cv-qualified T -- store/load/exchange/etc. below operate on
  // value_type (the cv-unqualified value representation), while __ptr_ below preserves _Tp's
  // cv-qualification so that constness/volatility of the *referenced* object is enforced by the
  // compiler (a const _Tp* can't be written through) and observed by the atomic builtins.
  using value_type = __remove_cv_t<_Tp>;

private:
  _LIBCPP_HIDE_FROM_ABI static value_type* __clear_padding(value_type& __val) noexcept {
    value_type* __ptr = std::addressof(__val);
#  if __has_builtin(__builtin_clear_padding)
    __builtin_clear_padding(__ptr);
#  endif
    return __ptr;
  }

  _LIBCPP_HIDE_FROM_ABI static bool __compare_exchange(
      _Tp* __ptr,
      value_type* __expected,
      value_type* __desired,
      bool __is_weak,
      int __success,
      int __failure) noexcept {
    if constexpr (
#  if __has_builtin(__builtin_clear_padding)
        has_unique_object_representations_v<value_type> || floating_point<value_type>
#  else
        true // NOLINT(readability-simplify-boolean-expr)
#  endif
    ) {
      return __atomic_compare_exchange(__ptr, __expected, __desired, __is_weak, __success, __failure);
    } else { // value_type has padding bits and __builtin_clear_padding is available
      __clear_padding(*__desired);
      value_type __copy = *__expected;
      __clear_padding(__copy);
      // The algorithm we use here is basically to perform `__atomic_compare_exchange` on the
      // values until it has either succeeded, or failed because the value representation of the
      // objects involved was different. This is why we loop around __atomic_compare_exchange:
      // we basically loop until its failure is caused by the value representation of the objects
      // being different, not only their object representation.
      while (true) {
        value_type __prev = __copy;
        if (__atomic_compare_exchange(__ptr, std::addressof(__copy), __desired, __is_weak, __success, __failure)) {
          return true;
        }
        value_type __curr = __copy;
        if (std::memcmp(__clear_padding(__prev), __clear_padding(__curr), sizeof(value_type)) != 0) {
          // Value representation without padding bits do not compare equal ->
          // write the current content of *ptr into *expected
          std::memcpy(__expected, std::addressof(__copy), sizeof(value_type));
          return false;
        }
      }
    }
  }

  friend struct __atomic_waitable_traits<__atomic_ref_base<_Tp>>;

  // require types that are 1, 2, 4, 8, or 16 bytes in length to be aligned to at least their size to be potentially
  // used lock-free
  static constexpr size_t __min_alignment = (sizeof(_Tp) & (sizeof(_Tp) - 1)) || (sizeof(_Tp) > 16) ? 0 : sizeof(_Tp);

public:
  static constexpr size_t required_alignment = alignof(_Tp) > __min_alignment ? alignof(_Tp) : __min_alignment;

  // The __atomic_always_lock_free builtin takes into account the alignment of the pointer if provided,
  // so we create a fake pointer with a suitable alignment when querying it. Note that we are guaranteed
  // that the pointer is going to be aligned properly at runtime because that is a (checked) precondition
  // of atomic_ref's constructor.
  static constexpr bool is_always_lock_free =
      __atomic_always_lock_free(sizeof(_Tp), std::addressof(__get_aligner_instance<required_alignment>::__instance));

  // P3323R1: volatile T is only supported when the specialization is always lock-free -- there's no
  // well-defined "which lock" story for a volatile object that outside code might also access directly.
  static_assert(is_always_lock_free || !is_volatile_v<_Tp>,
                "std::atomic_ref<T>: a volatile-qualified referenced type requires the atomic_ref "
                "specialization to always be lock-free");

  _LIBCPP_HIDE_FROM_ABI bool is_lock_free() const noexcept { return __atomic_is_lock_free(sizeof(_Tp), __ptr_); }

  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 void
  store(value_type __desired, memory_order __order = memory_order::seq_cst) const noexcept
    requires(!is_const_v<_Tp>)
  _LIBCPP_CHECK_STORE_MEMORY_ORDER(__order) {
#  if _LIBCPP_STD_VER >= 26
    if consteval {
      *__ptr_ = __desired;
      return;
    }
#  endif
    _LIBCPP_ASSERT_ARGUMENT_WITHIN_DOMAIN(
        __order == memory_order::relaxed || __order == memory_order::release || __order == memory_order::seq_cst,
        "atomic_ref: memory order argument to atomic store operation is invalid");
    __atomic_store(__ptr_, __clear_padding(__desired), std::__to_gcc_order(__order));
  }

  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 value_type operator=(value_type __desired) const noexcept
    requires(!is_const_v<_Tp>)
  {
    store(__desired);
    return __desired;
  }

  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 value_type
  load(memory_order __order = memory_order::seq_cst) const noexcept _LIBCPP_CHECK_LOAD_MEMORY_ORDER(__order) {
#  if _LIBCPP_STD_VER >= 26
    if consteval {
      return *__ptr_;
    }
#  endif
    _LIBCPP_ASSERT_ARGUMENT_WITHIN_DOMAIN(
        __order == memory_order::relaxed || __order == memory_order::consume || __order == memory_order::acquire ||
            __order == memory_order::seq_cst,
        "atomic_ref: memory order argument to atomic load operation is invalid");
    alignas(value_type) byte __mem[sizeof(value_type)];
    auto* __ret = reinterpret_cast<value_type*>(__mem);
    __atomic_load(__ptr_, __ret, std::__to_gcc_order(__order));
    return *__ret;
  }

  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 operator value_type() const noexcept { return load(); }

  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 value_type
  exchange(value_type __desired, memory_order __order = memory_order::seq_cst) const noexcept
    requires(!is_const_v<_Tp>)
  {
#  if _LIBCPP_STD_VER >= 26
    if consteval {
      value_type __old = *__ptr_;
      *__ptr_          = __desired;
      return __old;
    }
#  endif
    alignas(value_type) byte __mem[sizeof(value_type)];
    auto* __ret = reinterpret_cast<value_type*>(__mem);
    __atomic_exchange(__ptr_, __clear_padding(__desired), __ret, std::__to_gcc_order(__order));
    return *__ret;
  }

  // The consteval branches below are gated on is_scalar_v<_Tp>: they compare via `==`, which
  // -- unlike __compare_exchange's bytewise __atomic_compare_exchange/__clear_padding/memcmp
  // machinery below (none of which is usable in a constant expression) -- requires an equality
  // comparison to exist, so this doesn't extend to arbitrary trivially copyable class types.
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 bool compare_exchange_weak(
      value_type& __expected, value_type __desired, memory_order __success, memory_order __failure) const noexcept
    requires(!is_const_v<_Tp>)
  _LIBCPP_CHECK_EXCHANGE_MEMORY_ORDER(__success, __failure) {
#  if _LIBCPP_STD_VER >= 26
    if constexpr (is_scalar_v<_Tp>) {
      if consteval {
        if (*__ptr_ == __expected) {
          *__ptr_ = __desired;
          return true;
        }
        __expected = *__ptr_;
        return false;
      }
    }
#  endif
    _LIBCPP_ASSERT_ARGUMENT_WITHIN_DOMAIN(
        __failure == memory_order::relaxed || __failure == memory_order::consume ||
            __failure == memory_order::acquire || __failure == memory_order::seq_cst,
        "atomic_ref: failure memory order argument to weak atomic compare-and-exchange operation is invalid");
    return __compare_exchange(
        __ptr_,
        std::addressof(__expected),
        std::addressof(__desired),
        true,
        std::__to_gcc_order(__success),
        std::__to_gcc_order(__failure));
  }
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 bool compare_exchange_strong(
      value_type& __expected, value_type __desired, memory_order __success, memory_order __failure) const noexcept
    requires(!is_const_v<_Tp>)
  _LIBCPP_CHECK_EXCHANGE_MEMORY_ORDER(__success, __failure) {
#  if _LIBCPP_STD_VER >= 26
    if constexpr (is_scalar_v<_Tp>) {
      if consteval {
        if (*__ptr_ == __expected) {
          *__ptr_ = __desired;
          return true;
        }
        __expected = *__ptr_;
        return false;
      }
    }
#  endif
    _LIBCPP_ASSERT_ARGUMENT_WITHIN_DOMAIN(
        __failure == memory_order::relaxed || __failure == memory_order::consume ||
            __failure == memory_order::acquire || __failure == memory_order::seq_cst,
        "atomic_ref: failure memory order argument to strong atomic compare-and-exchange operation is invalid");
    return __compare_exchange(
        __ptr_,
        std::addressof(__expected),
        std::addressof(__desired),
        false,
        std::__to_gcc_order(__success),
        std::__to_gcc_order(__failure));
  }

  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 bool compare_exchange_weak(
      value_type& __expected, value_type __desired, memory_order __order = memory_order::seq_cst) const noexcept
    requires(!is_const_v<_Tp>)
  {
#  if _LIBCPP_STD_VER >= 26
    if constexpr (is_scalar_v<_Tp>) {
      if consteval {
        if (*__ptr_ == __expected) {
          *__ptr_ = __desired;
          return true;
        }
        __expected = *__ptr_;
        return false;
      }
    }
#  endif
    return __compare_exchange(
        __ptr_,
        std::addressof(__expected),
        std::addressof(__desired),
        true,
        std::__to_gcc_order(__order),
        std::__to_gcc_failure_order(__order));
  }
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 bool compare_exchange_strong(
      value_type& __expected, value_type __desired, memory_order __order = memory_order::seq_cst) const noexcept
    requires(!is_const_v<_Tp>)
  {
#  if _LIBCPP_STD_VER >= 26
    if constexpr (is_scalar_v<_Tp>) {
      if consteval {
        if (*__ptr_ == __expected) {
          *__ptr_ = __desired;
          return true;
        }
        __expected = *__ptr_;
        return false;
      }
    }
#  endif
    return __compare_exchange(
        __ptr_,
        std::addressof(__expected),
        std::addressof(__desired),
        false,
        std::__to_gcc_order(__order),
        std::__to_gcc_failure_order(__order));
  }

  // The consteval branch below calls this->load() directly instead of going through
  // std::__atomic_wait (kept non-constexpr, like atomic.h's wait() -- see its comment: Clang
  // eagerly instantiates constexpr function templates, which is incompatible with
  // atomic_flag.h's fragile pre-specialization use of the shared std::__atomic_wait).
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 void
  wait(value_type __old, memory_order __order = memory_order::seq_cst) const noexcept
      _LIBCPP_CHECK_WAIT_MEMORY_ORDER(__order) {
#  if _LIBCPP_STD_VER >= 26
    if constexpr (is_scalar_v<_Tp>) {
      if consteval {
        while (this->load(__order) == __old) {
        }
        return;
      }
    }
#  endif
    _LIBCPP_ASSERT_ARGUMENT_WITHIN_DOMAIN(
        __order == memory_order::relaxed || __order == memory_order::consume || __order == memory_order::acquire ||
            __order == memory_order::seq_cst,
        "atomic_ref: memory order argument to atomic wait operation is invalid");
    std::__atomic_wait(*this, __old, __order);
  }
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 void notify_one() const noexcept
    requires(!is_const_v<_Tp>)
  {
#  if _LIBCPP_STD_VER >= 26
    if consteval {
      return; // no-op: constant evaluation is single-threaded, so nothing can be waiting.
    }
#  endif
    std::__atomic_notify_one(*this);
  }
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 void notify_all() const noexcept
    requires(!is_const_v<_Tp>)
  {
#  if _LIBCPP_STD_VER >= 26
    if consteval {
      return; // no-op: constant evaluation is single-threaded, so nothing can be waiting.
    }
#  endif
    std::__atomic_notify_all(*this);
  }

#  if _LIBCPP_STD_VER >= 26
  _LIBCPP_HIDE_FROM_ABI constexpr _Tp* address() const noexcept { return __ptr_; }
#  endif // _LIBCPP_STD_VER >= 26

protected:
  using _Aligned_Tp [[__gnu__::__aligned__(required_alignment), __gnu__::__nodebug__]] = _Tp;
  _Aligned_Tp* __ptr_;

  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 __atomic_ref_base(_Tp& __obj) : __ptr_(std::addressof(__obj)) {}
};

template <class _Tp>
struct __atomic_waitable_traits<__atomic_ref_base<_Tp>> {
  static _LIBCPP_HIDE_FROM_ABI typename __atomic_ref_base<_Tp>::value_type
  __atomic_load(const __atomic_ref_base<_Tp>& __a, memory_order __order) {
    return __a.load(__order);
  }
  static _LIBCPP_HIDE_FROM_ABI const _Tp* __atomic_contention_address(const __atomic_ref_base<_Tp>& __a) {
    return __a.__ptr_;
  }
};

template <class _Tp>
struct atomic_ref : public __atomic_ref_base<_Tp> {
  static_assert(is_trivially_copyable_v<_Tp>, "std::atomic_ref<T> requires that 'T' be a trivially copyable type");

  using __base _LIBCPP_NODEBUG = __atomic_ref_base<_Tp>;

  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 explicit atomic_ref(_Tp& __obj) : __base(__obj) {
    // The alignment check below reinterpret_casts the referenced object's address to check its
    // low bits, which isn't usable in a constant expression -- but it's also unnecessary there:
    // the constant evaluator already guarantees `__obj` is a real, fully-aligned object of type
    // `_Tp`, so there's nothing an alignment precondition check could catch at compile time.
#  if _LIBCPP_STD_VER >= 26
    if (!__libcpp_is_constant_evaluated())
#  endif
      _LIBCPP_ASSERT_ARGUMENT_WITHIN_DOMAIN(
          reinterpret_cast<uintptr_t>(std::addressof(__obj)) % __base::required_alignment == 0,
          "atomic_ref ctor: referenced object must be aligned to required_alignment");
  }

  _LIBCPP_HIDE_FROM_ABI atomic_ref(const atomic_ref&) noexcept = default;

  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 __base::value_type
  operator=(__base::value_type __desired) const noexcept
    requires(!is_const_v<_Tp>)
  {
    return __base::operator=(__desired);
  }

  atomic_ref& operator=(const atomic_ref&) = delete;
};

template <class _Tp>
// P3323R1: "for all integral types except cv bool" -- same_as<bool, _Tp> alone only excludes
// exactly bool, not const/volatile bool, so cv bool must be stripped before the comparison or
// atomic_ref<const bool> would wrongly pick up integral RMW operations bool doesn't have.
  requires(std::integral<_Tp> && !std::same_as<bool, __remove_cv_t<_Tp>>)
struct atomic_ref<_Tp> : public __atomic_ref_base<_Tp> {
  using __base _LIBCPP_NODEBUG     = __atomic_ref_base<_Tp>;
  using value_type _LIBCPP_NODEBUG = __base::value_type;

  using difference_type = value_type;

  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 explicit atomic_ref(_Tp& __obj) : __base(__obj) {
    // The alignment check below reinterpret_casts the referenced object's address to check its
    // low bits, which isn't usable in a constant expression -- but it's also unnecessary there:
    // the constant evaluator already guarantees `__obj` is a real, fully-aligned object of type
    // `_Tp`, so there's nothing an alignment precondition check could catch at compile time.
#  if _LIBCPP_STD_VER >= 26
    if (!__libcpp_is_constant_evaluated())
#  endif
      _LIBCPP_ASSERT_ARGUMENT_WITHIN_DOMAIN(
          reinterpret_cast<uintptr_t>(std::addressof(__obj)) % __base::required_alignment == 0,
          "atomic_ref ctor: referenced object must be aligned to required_alignment");
  }

  _LIBCPP_HIDE_FROM_ABI atomic_ref(const atomic_ref&) noexcept = default;

  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 value_type operator=(value_type __desired) const noexcept
    requires(!is_const_v<_Tp>)
  {
    return __base::operator=(__desired);
  }

  atomic_ref& operator=(const atomic_ref&) = delete;

  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 value_type
  fetch_add(value_type __arg, memory_order __order = memory_order_seq_cst) const noexcept
    requires(!is_const_v<_Tp>)
  {
#  if _LIBCPP_STD_VER >= 26
    if consteval {
      value_type __old = *this->__ptr_;
      *this->__ptr_    = static_cast<value_type>(__old + __arg);
      return __old;
    }
#  endif
    return __atomic_fetch_add(this->__ptr_, __arg, std::__to_gcc_order(__order));
  }
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 value_type
  fetch_sub(value_type __arg, memory_order __order = memory_order_seq_cst) const noexcept
    requires(!is_const_v<_Tp>)
  {
#  if _LIBCPP_STD_VER >= 26
    if consteval {
      value_type __old = *this->__ptr_;
      *this->__ptr_    = static_cast<value_type>(__old - __arg);
      return __old;
    }
#  endif
    return __atomic_fetch_sub(this->__ptr_, __arg, std::__to_gcc_order(__order));
  }
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 value_type
  fetch_and(value_type __arg, memory_order __order = memory_order_seq_cst) const noexcept
    requires(!is_const_v<_Tp>)
  {
#  if _LIBCPP_STD_VER >= 26
    if consteval {
      value_type __old = *this->__ptr_;
      *this->__ptr_    = static_cast<value_type>(__old & __arg);
      return __old;
    }
#  endif
    return __atomic_fetch_and(this->__ptr_, __arg, std::__to_gcc_order(__order));
  }
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 value_type
  fetch_or(value_type __arg, memory_order __order = memory_order_seq_cst) const noexcept
    requires(!is_const_v<_Tp>)
  {
#  if _LIBCPP_STD_VER >= 26
    if consteval {
      value_type __old = *this->__ptr_;
      *this->__ptr_    = static_cast<value_type>(__old | __arg);
      return __old;
    }
#  endif
    return __atomic_fetch_or(this->__ptr_, __arg, std::__to_gcc_order(__order));
  }
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 value_type
  fetch_xor(value_type __arg, memory_order __order = memory_order_seq_cst) const noexcept
    requires(!is_const_v<_Tp>)
  {
#  if _LIBCPP_STD_VER >= 26
    if consteval {
      value_type __old = *this->__ptr_;
      *this->__ptr_    = static_cast<value_type>(__old ^ __arg);
      return __old;
    }
#  endif
    return __atomic_fetch_xor(this->__ptr_, __arg, std::__to_gcc_order(__order));
  }
#  if _LIBCPP_STD_VER >= 26
  _LIBCPP_HIDE_FROM_ABI constexpr value_type
  fetch_max(value_type __arg, memory_order __order = memory_order_seq_cst) const noexcept
    requires(!is_const_v<_Tp>)
  {
    if consteval {
      value_type __old = *this->__ptr_;
      *this->__ptr_    = __old < __arg ? __arg : __old;
      return __old;
    }
    return __atomic_fetch_max(this->__ptr_, __arg, std::__to_gcc_order(__order));
  }
  _LIBCPP_HIDE_FROM_ABI constexpr value_type
  fetch_min(value_type __arg, memory_order __order = memory_order_seq_cst) const noexcept
    requires(!is_const_v<_Tp>)
  {
    if consteval {
      value_type __old = *this->__ptr_;
      *this->__ptr_    = __arg < __old ? __arg : __old;
      return __old;
    }
    return __atomic_fetch_min(this->__ptr_, __arg, std::__to_gcc_order(__order));
  }
#  endif // _LIBCPP_STD_VER >= 26

  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 value_type operator++(int) const noexcept
    requires(!is_const_v<_Tp>)
  {
    return fetch_add(value_type(1));
  }
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 value_type operator--(int) const noexcept
    requires(!is_const_v<_Tp>)
  {
    return fetch_sub(value_type(1));
  }
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 value_type operator++() const noexcept
    requires(!is_const_v<_Tp>)
  {
    return fetch_add(value_type(1)) + value_type(1);
  }
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 value_type operator--() const noexcept
    requires(!is_const_v<_Tp>)
  {
    return fetch_sub(value_type(1)) - value_type(1);
  }
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 value_type operator+=(value_type __arg) const noexcept
    requires(!is_const_v<_Tp>)
  {
    return fetch_add(__arg) + __arg;
  }
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 value_type operator-=(value_type __arg) const noexcept
    requires(!is_const_v<_Tp>)
  {
    return fetch_sub(__arg) - __arg;
  }
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 value_type operator&=(value_type __arg) const noexcept
    requires(!is_const_v<_Tp>)
  {
    return fetch_and(__arg) & __arg;
  }
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 value_type operator|=(value_type __arg) const noexcept
    requires(!is_const_v<_Tp>)
  {
    return fetch_or(__arg) | __arg;
  }
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 value_type operator^=(value_type __arg) const noexcept
    requires(!is_const_v<_Tp>)
  {
    return fetch_xor(__arg) ^ __arg;
  }
};

template <class _Tp>
  requires std::floating_point<_Tp>
struct atomic_ref<_Tp> : public __atomic_ref_base<_Tp> {
  using __base _LIBCPP_NODEBUG     = __atomic_ref_base<_Tp>;
  using value_type _LIBCPP_NODEBUG = __base::value_type;

  using difference_type = value_type;

  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 explicit atomic_ref(_Tp& __obj) : __base(__obj) {
    // The alignment check below reinterpret_casts the referenced object's address to check its
    // low bits, which isn't usable in a constant expression -- but it's also unnecessary there:
    // the constant evaluator already guarantees `__obj` is a real, fully-aligned object of type
    // `_Tp`, so there's nothing an alignment precondition check could catch at compile time.
#  if _LIBCPP_STD_VER >= 26
    if (!__libcpp_is_constant_evaluated())
#  endif
      _LIBCPP_ASSERT_ARGUMENT_WITHIN_DOMAIN(
          reinterpret_cast<uintptr_t>(std::addressof(__obj)) % __base::required_alignment == 0,
          "atomic_ref ctor: referenced object must be aligned to required_alignment");
  }

  _LIBCPP_HIDE_FROM_ABI atomic_ref(const atomic_ref&) noexcept = default;

  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 value_type operator=(value_type __desired) const noexcept
    requires(!is_const_v<_Tp>)
  {
    return __base::operator=(__desired);
  }

  atomic_ref& operator=(const atomic_ref&) = delete;

  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 value_type
  fetch_add(value_type __arg, memory_order __order = memory_order_seq_cst) const noexcept
    requires(!is_const_v<_Tp>)
  {
    value_type __old = this->load(memory_order_relaxed);
    value_type __new = __old + __arg;
    while (!this->compare_exchange_weak(__old, __new, __order, memory_order_relaxed)) {
      __new = __old + __arg;
    }
    return __old;
  }
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 value_type
  fetch_sub(value_type __arg, memory_order __order = memory_order_seq_cst) const noexcept
    requires(!is_const_v<_Tp>)
  {
    value_type __old = this->load(memory_order_relaxed);
    value_type __new = __old - __arg;
    while (!this->compare_exchange_weak(__old, __new, __order, memory_order_relaxed)) {
      __new = __old - __arg;
    }
    return __old;
  }

#  if _LIBCPP_STD_VER >= 26
  // As if by fmaximum_num/fminimum_num: unlike a plain `<`-based max/min, NaN never propagates into the
  // stored value. If both operands are NaN, or exactly one is, an unspecified NaN value (either operand)
  // is stored, matching [atomics.ref.float]'s "unspecified which" wording.
  _LIBCPP_HIDE_FROM_ABI static constexpr value_type __maximum_num(value_type __a, value_type __b) {
    if (__builtin_isnan(__a))
      return __b;
    if (__builtin_isnan(__b))
      return __a;
    return __a < __b ? __b : __a;
  }
  _LIBCPP_HIDE_FROM_ABI static constexpr value_type __minimum_num(value_type __a, value_type __b) {
    if (__builtin_isnan(__a))
      return __b;
    if (__builtin_isnan(__b))
      return __a;
    return __b < __a ? __b : __a;
  }

  _LIBCPP_HIDE_FROM_ABI constexpr value_type
  fetch_max(value_type __arg, memory_order __order = memory_order_seq_cst) const noexcept
    requires(!is_const_v<_Tp>)
  {
    value_type __old = this->load(memory_order_relaxed);
    value_type __new = __maximum_num(__old, __arg);
    while (!this->compare_exchange_weak(__old, __new, __order, memory_order_relaxed)) {
      __new = __maximum_num(__old, __arg);
    }
    return __old;
  }
  _LIBCPP_HIDE_FROM_ABI constexpr value_type
  fetch_min(value_type __arg, memory_order __order = memory_order_seq_cst) const noexcept
    requires(!is_const_v<_Tp>)
  {
    value_type __old = this->load(memory_order_relaxed);
    value_type __new = __minimum_num(__old, __arg);
    while (!this->compare_exchange_weak(__old, __new, __order, memory_order_relaxed)) {
      __new = __minimum_num(__old, __arg);
    }
    return __old;
  }
#  endif // _LIBCPP_STD_VER >= 26

  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 value_type operator+=(value_type __arg) const noexcept
    requires(!is_const_v<_Tp>)
  {
    return fetch_add(__arg) + __arg;
  }
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 value_type operator-=(value_type __arg) const noexcept
    requires(!is_const_v<_Tp>)
  {
    return fetch_sub(__arg) - __arg;
  }
};

// P3323R1 removed [atomics.ref.pointer]'s "template<class T> struct atomic_ref<T*>" deduced-pattern
// notation entirely, replacing it with the same "for all pointer-to-object types" / placeholder-type
// convention already used for [atomics.ref.int] and [atomics.ref.float] (each paired with a
// value_type = remove_cv_t<placeholder> and is_const_v<placeholder>-gated Constraints). A literal C++
// partial specialization `atomic_ref<_Tp*>` can never match a cv-qualified pointer argument like
// `int* const` (the top-level cv would have to be part of the pattern itself), so following the old
// notation verbatim would keep silently excluding cv-qualified pointers from this specialization --
// unlike atomic_ref<const int>, which already reaches [atomics.ref.int] because std::integral<const
// int> strips cv. Constraining on is_pointer_v<_Tp> instead (_Tp now the *whole*, possibly
// cv-qualified, pointer type -- mirroring the requires(integral<_Tp> && ...) and requires
// floating_point<_Tp> specializations) matches that same treatment.
template <class _Tp>
  requires is_pointer_v<_Tp>
struct atomic_ref<_Tp> : public __atomic_ref_base<_Tp> {
  using __base _LIBCPP_NODEBUG     = __atomic_ref_base<_Tp>;
  using value_type _LIBCPP_NODEBUG = __base::value_type;
  using __pointee _LIBCPP_NODEBUG  = remove_pointer_t<value_type>;

  using difference_type = ptrdiff_t;

  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 explicit atomic_ref(_Tp& __obj) : __base(__obj) {}

  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 value_type operator=(value_type __desired) const noexcept
    requires(!is_const_v<_Tp>)
  {
    return __base::operator=(__desired);
  }

  atomic_ref& operator=(const atomic_ref&) = delete;

  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 value_type
  fetch_add(ptrdiff_t __arg, memory_order __order = memory_order_seq_cst) const noexcept
    requires(!is_const_v<_Tp>)
  {
#  if _LIBCPP_STD_VER >= 26
    if consteval {
      value_type __old = *this->__ptr_;
      *this->__ptr_    = __old + __arg;
      return __old;
    }
#  endif
    return __atomic_fetch_add(this->__ptr_, __arg * sizeof(__pointee), std::__to_gcc_order(__order));
  }
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 value_type
  fetch_sub(ptrdiff_t __arg, memory_order __order = memory_order_seq_cst) const noexcept
    requires(!is_const_v<_Tp>)
  {
#  if _LIBCPP_STD_VER >= 26
    if consteval {
      value_type __old = *this->__ptr_;
      *this->__ptr_    = __old - __arg;
      return __old;
    }
#  endif
    return __atomic_fetch_sub(this->__ptr_, __arg * sizeof(__pointee), std::__to_gcc_order(__order));
  }

  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 value_type operator++(int) const noexcept
    requires(!is_const_v<_Tp>)
  {
    return fetch_add(1);
  }
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 value_type operator--(int) const noexcept
    requires(!is_const_v<_Tp>)
  {
    return fetch_sub(1);
  }
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 value_type operator++() const noexcept
    requires(!is_const_v<_Tp>)
  {
    return fetch_add(1) + 1;
  }
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 value_type operator--() const noexcept
    requires(!is_const_v<_Tp>)
  {
    return fetch_sub(1) - 1;
  }
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 value_type operator+=(ptrdiff_t __arg) const noexcept
    requires(!is_const_v<_Tp>)
  {
    return fetch_add(__arg) + __arg;
  }
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 value_type operator-=(ptrdiff_t __arg) const noexcept
    requires(!is_const_v<_Tp>)
  {
    return fetch_sub(__arg) - __arg;
  }
};

_LIBCPP_CTAD_SUPPORTED_FOR_TYPE(atomic_ref);

#endif // _LIBCPP_STD_VER >= 20

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP__ATOMIC_ATOMIC_REF_H
