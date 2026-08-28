//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___ATOMIC_SUPPORT_C11_H
#define _LIBCPP___ATOMIC_SUPPORT_C11_H

#include <__atomic/memory_order.h>
#include <__config>
#include <__cstddef/ptrdiff_t.h>
#include <__memory/addressof.h>
#include <__type_traits/is_floating_point.h>
#include <__type_traits/is_scalar.h>
#include <__type_traits/remove_const.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

//
// This file implements support for C11-style atomics
//

_LIBCPP_BEGIN_NAMESPACE_STD

// P3309R3 support: none of the `__c11_atomic_*` builtins are usable in a constant
// expression (only `__c11_atomic_is_lock_free` is), so constant evaluation instead
// reads/writes `__a_value` directly, bypassing the builtins entirely. That direct
// access only type-checks for scalar element types -- `_Atomic(T)` for a class type
// `T` has no implicit conversion back to `T` -- so these branches are gated on
// `is_scalar_v<_Tp>` and fall through to the (non-constexpr) builtins otherwise.
// Memory order is meaningless without concurrency, so it's ignored in this branch,
// matching the paper's stated rationale.
template <typename _Tp>
struct __cxx_atomic_base_impl {
  _LIBCPP_HIDE_FROM_ABI
#ifndef _LIBCPP_CXX03_LANG
  __cxx_atomic_base_impl() _NOEXCEPT = default;
#else
  __cxx_atomic_base_impl() _NOEXCEPT : __a_value() {
  }
#endif // _LIBCPP_CXX03_LANG
  _LIBCPP_CONSTEXPR explicit __cxx_atomic_base_impl(_Tp __value) _NOEXCEPT : __a_value(__value) {}
  _Atomic(_Tp) __a_value;
};

#define __cxx_atomic_is_lock_free(__s) __c11_atomic_is_lock_free(__s)

_LIBCPP_HIDE_FROM_ABI inline void __cxx_atomic_thread_fence(memory_order __order) _NOEXCEPT {
  __c11_atomic_thread_fence(static_cast<__memory_order_underlying_t>(__order));
}

_LIBCPP_HIDE_FROM_ABI inline void __cxx_atomic_signal_fence(memory_order __order) _NOEXCEPT {
  __c11_atomic_signal_fence(static_cast<__memory_order_underlying_t>(__order));
}

template <class _Tp>
_LIBCPP_HIDE_FROM_ABI void __cxx_atomic_init(__cxx_atomic_base_impl<_Tp> volatile* __a, _Tp __val) _NOEXCEPT {
  __c11_atomic_init(std::addressof(__a->__a_value), __val);
}
template <class _Tp>
_LIBCPP_HIDE_FROM_ABI void __cxx_atomic_init(__cxx_atomic_base_impl<_Tp>* __a, _Tp __val) _NOEXCEPT {
  __c11_atomic_init(std::addressof(__a->__a_value), __val);
}

template <class _Tp>
_LIBCPP_HIDE_FROM_ABI void
__cxx_atomic_store(__cxx_atomic_base_impl<_Tp> volatile* __a, _Tp __val, memory_order __order) _NOEXCEPT {
  __c11_atomic_store(std::addressof(__a->__a_value), __val, static_cast<__memory_order_underlying_t>(__order));
}
template <class _Tp>
_LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 void
__cxx_atomic_store(__cxx_atomic_base_impl<_Tp>* __a, _Tp __val, memory_order __order) _NOEXCEPT {
#if _LIBCPP_STD_VER >= 26
  if constexpr (is_scalar_v<_Tp>) {
    if consteval {
      __a->__a_value = __val;
      return;
    }
  }
#endif
  __c11_atomic_store(std::addressof(__a->__a_value), __val, static_cast<__memory_order_underlying_t>(__order));
}

template <class _Tp>
_LIBCPP_HIDE_FROM_ABI _Tp
__cxx_atomic_load(__cxx_atomic_base_impl<_Tp> const volatile* __a, memory_order __order) _NOEXCEPT {
  using __ptr_type = __remove_const_t<decltype(__a->__a_value)>*;
  return __c11_atomic_load(
      const_cast<__ptr_type>(std::addressof(__a->__a_value)), static_cast<__memory_order_underlying_t>(__order));
}
template <class _Tp>
_LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp
__cxx_atomic_load(__cxx_atomic_base_impl<_Tp> const* __a, memory_order __order) _NOEXCEPT {
#if _LIBCPP_STD_VER >= 26
  if constexpr (is_scalar_v<_Tp>) {
    if consteval {
      return __a->__a_value;
    }
  }
#endif
  using __ptr_type = __remove_const_t<decltype(__a->__a_value)>*;
  return __c11_atomic_load(
      const_cast<__ptr_type>(std::addressof(__a->__a_value)), static_cast<__memory_order_underlying_t>(__order));
}

template <class _Tp>
_LIBCPP_HIDE_FROM_ABI void
__cxx_atomic_load_inplace(__cxx_atomic_base_impl<_Tp> const volatile* __a, _Tp* __dst, memory_order __order) _NOEXCEPT {
  using __ptr_type = __remove_const_t<decltype(__a->__a_value)>*;
  *__dst           = __c11_atomic_load(
      const_cast<__ptr_type>(std::addressof(__a->__a_value)), static_cast<__memory_order_underlying_t>(__order));
}
template <class _Tp>
_LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 void
__cxx_atomic_load_inplace(__cxx_atomic_base_impl<_Tp> const* __a, _Tp* __dst, memory_order __order) _NOEXCEPT {
#if _LIBCPP_STD_VER >= 26
  if constexpr (is_scalar_v<_Tp>) {
    if consteval {
      *__dst = __a->__a_value;
      return;
    }
  }
#endif
  using __ptr_type = __remove_const_t<decltype(__a->__a_value)>*;
  *__dst           = __c11_atomic_load(
      const_cast<__ptr_type>(std::addressof(__a->__a_value)), static_cast<__memory_order_underlying_t>(__order));
}

template <class _Tp>
_LIBCPP_HIDE_FROM_ABI _Tp
__cxx_atomic_exchange(__cxx_atomic_base_impl<_Tp> volatile* __a, _Tp __value, memory_order __order) _NOEXCEPT {
  return __c11_atomic_exchange(
      std::addressof(__a->__a_value), __value, static_cast<__memory_order_underlying_t>(__order));
}
template <class _Tp>
_LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp
__cxx_atomic_exchange(__cxx_atomic_base_impl<_Tp>* __a, _Tp __value, memory_order __order) _NOEXCEPT {
#if _LIBCPP_STD_VER >= 26
  if constexpr (is_scalar_v<_Tp>) {
    if consteval {
      _Tp __old      = __a->__a_value;
      __a->__a_value = __value;
      return __old;
    }
  }
#endif
  return __c11_atomic_exchange(
      std::addressof(__a->__a_value), __value, static_cast<__memory_order_underlying_t>(__order));
}

_LIBCPP_HIDE_FROM_ABI inline _LIBCPP_CONSTEXPR memory_order __to_failure_order(memory_order __order) {
  // Avoid switch statement to make this a constexpr.
  return __order == memory_order_release
           ? memory_order_relaxed
           : (__order == memory_order_acq_rel ? memory_order_acquire : __order);
}

template <class _Tp>
_LIBCPP_HIDE_FROM_ABI bool __cxx_atomic_compare_exchange_strong(
    __cxx_atomic_base_impl<_Tp> volatile* __a,
    _Tp* __expected,
    _Tp __value,
    memory_order __success,
    memory_order __failure) _NOEXCEPT {
  return __c11_atomic_compare_exchange_strong(
      std::addressof(__a->__a_value),
      __expected,
      __value,
      static_cast<__memory_order_underlying_t>(__success),
      static_cast<__memory_order_underlying_t>(__to_failure_order(__failure)));
}
template <class _Tp>
_LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 bool __cxx_atomic_compare_exchange_strong(
    __cxx_atomic_base_impl<_Tp>* __a, _Tp* __expected, _Tp __value, memory_order __success, memory_order __failure)
    _NOEXCEPT {
#if _LIBCPP_STD_VER >= 26
  if constexpr (is_scalar_v<_Tp>) {
    if consteval {
      if (__a->__a_value == *__expected) {
        __a->__a_value = __value;
        return true;
      }
      *__expected = __a->__a_value;
      return false;
    }
  }
#endif
  return __c11_atomic_compare_exchange_strong(
      std::addressof(__a->__a_value),
      __expected,
      __value,
      static_cast<__memory_order_underlying_t>(__success),
      static_cast<__memory_order_underlying_t>(__to_failure_order(__failure)));
}

template <class _Tp>
_LIBCPP_HIDE_FROM_ABI bool __cxx_atomic_compare_exchange_weak(
    __cxx_atomic_base_impl<_Tp> volatile* __a,
    _Tp* __expected,
    _Tp __value,
    memory_order __success,
    memory_order __failure) _NOEXCEPT {
  return __c11_atomic_compare_exchange_weak(
      std::addressof(__a->__a_value),
      __expected,
      __value,
      static_cast<__memory_order_underlying_t>(__success),
      static_cast<__memory_order_underlying_t>(__to_failure_order(__failure)));
}
template <class _Tp>
_LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 bool __cxx_atomic_compare_exchange_weak(
    __cxx_atomic_base_impl<_Tp>* __a, _Tp* __expected, _Tp __value, memory_order __success, memory_order __failure)
    _NOEXCEPT {
#if _LIBCPP_STD_VER >= 26
  if constexpr (is_scalar_v<_Tp>) {
    if consteval {
      // Constant evaluation is single-threaded, so a weak CAS never needs to
      // spuriously fail (permitted, not required, by [atomics.types.operations]).
      if (__a->__a_value == *__expected) {
        __a->__a_value = __value;
        return true;
      }
      *__expected = __a->__a_value;
      return false;
    }
  }
#endif
  return __c11_atomic_compare_exchange_weak(
      std::addressof(__a->__a_value),
      __expected,
      __value,
      static_cast<__memory_order_underlying_t>(__success),
      static_cast<__memory_order_underlying_t>(__to_failure_order(__failure)));
}

template <class _Tp>
_LIBCPP_HIDE_FROM_ABI _Tp
__cxx_atomic_fetch_add(__cxx_atomic_base_impl<_Tp> volatile* __a, _Tp __delta, memory_order __order) _NOEXCEPT {
  return __c11_atomic_fetch_add(
      std::addressof(__a->__a_value), __delta, static_cast<__memory_order_underlying_t>(__order));
}
template <class _Tp>
_LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp
__cxx_atomic_fetch_add(__cxx_atomic_base_impl<_Tp>* __a, _Tp __delta, memory_order __order) _NOEXCEPT {
#if _LIBCPP_STD_VER >= 26
  if constexpr (is_scalar_v<_Tp>) {
    if consteval {
      _Tp __old      = __a->__a_value;
      __a->__a_value = static_cast<_Tp>(__old + __delta);
      return __old;
    }
  }
#endif
  return __c11_atomic_fetch_add(
      std::addressof(__a->__a_value), __delta, static_cast<__memory_order_underlying_t>(__order));
}

template <class _Tp>
_LIBCPP_HIDE_FROM_ABI _Tp*
__cxx_atomic_fetch_add(__cxx_atomic_base_impl<_Tp*> volatile* __a, ptrdiff_t __delta, memory_order __order) _NOEXCEPT {
  return __c11_atomic_fetch_add(
      std::addressof(__a->__a_value), __delta, static_cast<__memory_order_underlying_t>(__order));
}
template <class _Tp>
_LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp*
__cxx_atomic_fetch_add(__cxx_atomic_base_impl<_Tp*>* __a, ptrdiff_t __delta, memory_order __order) _NOEXCEPT {
#if _LIBCPP_STD_VER >= 26
  if consteval {
    _Tp* __old     = __a->__a_value;
    __a->__a_value = __old + __delta;
    return __old;
  }
#endif
  return __c11_atomic_fetch_add(
      std::addressof(__a->__a_value), __delta, static_cast<__memory_order_underlying_t>(__order));
}

template <class _Tp>
_LIBCPP_HIDE_FROM_ABI _Tp
__cxx_atomic_fetch_sub(__cxx_atomic_base_impl<_Tp> volatile* __a, _Tp __delta, memory_order __order) _NOEXCEPT {
  return __c11_atomic_fetch_sub(
      std::addressof(__a->__a_value), __delta, static_cast<__memory_order_underlying_t>(__order));
}
template <class _Tp>
_LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp
__cxx_atomic_fetch_sub(__cxx_atomic_base_impl<_Tp>* __a, _Tp __delta, memory_order __order) _NOEXCEPT {
#if _LIBCPP_STD_VER >= 26
  if constexpr (is_scalar_v<_Tp>) {
    if consteval {
      _Tp __old      = __a->__a_value;
      __a->__a_value = static_cast<_Tp>(__old - __delta);
      return __old;
    }
  }
#endif
  return __c11_atomic_fetch_sub(
      std::addressof(__a->__a_value), __delta, static_cast<__memory_order_underlying_t>(__order));
}
template <class _Tp>
_LIBCPP_HIDE_FROM_ABI _Tp*
__cxx_atomic_fetch_sub(__cxx_atomic_base_impl<_Tp*> volatile* __a, ptrdiff_t __delta, memory_order __order) _NOEXCEPT {
  return __c11_atomic_fetch_sub(
      std::addressof(__a->__a_value), __delta, static_cast<__memory_order_underlying_t>(__order));
}
template <class _Tp>
_LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp*
__cxx_atomic_fetch_sub(__cxx_atomic_base_impl<_Tp*>* __a, ptrdiff_t __delta, memory_order __order) _NOEXCEPT {
#if _LIBCPP_STD_VER >= 26
  if consteval {
    _Tp* __old     = __a->__a_value;
    __a->__a_value = __old - __delta;
    return __old;
  }
#endif
  return __c11_atomic_fetch_sub(
      std::addressof(__a->__a_value), __delta, static_cast<__memory_order_underlying_t>(__order));
}

template <class _Tp>
_LIBCPP_HIDE_FROM_ABI _Tp
__cxx_atomic_fetch_max(__cxx_atomic_base_impl<_Tp> volatile* __a, _Tp __value, memory_order __order) _NOEXCEPT {
  return __c11_atomic_fetch_max(
      std::addressof(__a->__a_value), __value, static_cast<__memory_order_underlying_t>(__order));
}
#if _LIBCPP_STD_VER >= 26
// NaN never propagates into the stored value, matching [atomics.types.float]'s "unspecified
// which" wording -- as if by fmaximum_num/fminimum_num. Duplicated (rather than shared) from
// atomic<floating-point>::__maximum_num/__minimum_num in <atomic>: this consteval branch is the
// single call site reached by both the integral fetch_max/fetch_min path and, via
// atomic<floating-point>::__rmw_op's builtin_op lambda, the floating-point path, so it can't
// itself depend on that class-local helper.
template <class _Tp>
_LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp __cxx_atomic_consteval_maximum_num(_Tp __a, _Tp __b) {
  if constexpr (is_floating_point_v<_Tp>) {
    if (__builtin_isnan(__a))
      return __b;
    if (__builtin_isnan(__b))
      return __a;
  }
  return __a < __b ? __b : __a;
}
template <class _Tp>
_LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp __cxx_atomic_consteval_minimum_num(_Tp __a, _Tp __b) {
  if constexpr (is_floating_point_v<_Tp>) {
    if (__builtin_isnan(__a))
      return __b;
    if (__builtin_isnan(__b))
      return __a;
  }
  return __b < __a ? __b : __a;
}
#endif // _LIBCPP_STD_VER >= 26

template <class _Tp>
_LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp
__cxx_atomic_fetch_max(__cxx_atomic_base_impl<_Tp>* __a, _Tp __value, memory_order __order) _NOEXCEPT {
#if _LIBCPP_STD_VER >= 26
  if constexpr (is_scalar_v<_Tp>) {
    if consteval {
      _Tp __old      = __a->__a_value;
      __a->__a_value = std::__cxx_atomic_consteval_maximum_num(__old, __value);
      return __old;
    }
  }
#endif
  return __c11_atomic_fetch_max(
      std::addressof(__a->__a_value), __value, static_cast<__memory_order_underlying_t>(__order));
}

template <class _Tp>
_LIBCPP_HIDE_FROM_ABI _Tp
__cxx_atomic_fetch_min(__cxx_atomic_base_impl<_Tp> volatile* __a, _Tp __value, memory_order __order) _NOEXCEPT {
  return __c11_atomic_fetch_min(
      std::addressof(__a->__a_value), __value, static_cast<__memory_order_underlying_t>(__order));
}
template <class _Tp>
_LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp
__cxx_atomic_fetch_min(__cxx_atomic_base_impl<_Tp>* __a, _Tp __value, memory_order __order) _NOEXCEPT {
#if _LIBCPP_STD_VER >= 26
  if constexpr (is_scalar_v<_Tp>) {
    if consteval {
      _Tp __old      = __a->__a_value;
      __a->__a_value = std::__cxx_atomic_consteval_minimum_num(__old, __value);
      return __old;
    }
  }
#endif
  return __c11_atomic_fetch_min(
      std::addressof(__a->__a_value), __value, static_cast<__memory_order_underlying_t>(__order));
}

template <class _Tp>
_LIBCPP_HIDE_FROM_ABI _Tp
__cxx_atomic_fetch_and(__cxx_atomic_base_impl<_Tp> volatile* __a, _Tp __pattern, memory_order __order) _NOEXCEPT {
  return __c11_atomic_fetch_and(
      std::addressof(__a->__a_value), __pattern, static_cast<__memory_order_underlying_t>(__order));
}
template <class _Tp>
_LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp
__cxx_atomic_fetch_and(__cxx_atomic_base_impl<_Tp>* __a, _Tp __pattern, memory_order __order) _NOEXCEPT {
#if _LIBCPP_STD_VER >= 26
  if constexpr (is_scalar_v<_Tp>) {
    if consteval {
      _Tp __old      = __a->__a_value;
      __a->__a_value = static_cast<_Tp>(__old & __pattern);
      return __old;
    }
  }
#endif
  return __c11_atomic_fetch_and(
      std::addressof(__a->__a_value), __pattern, static_cast<__memory_order_underlying_t>(__order));
}

template <class _Tp>
_LIBCPP_HIDE_FROM_ABI _Tp
__cxx_atomic_fetch_or(__cxx_atomic_base_impl<_Tp> volatile* __a, _Tp __pattern, memory_order __order) _NOEXCEPT {
  return __c11_atomic_fetch_or(
      std::addressof(__a->__a_value), __pattern, static_cast<__memory_order_underlying_t>(__order));
}
template <class _Tp>
_LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp
__cxx_atomic_fetch_or(__cxx_atomic_base_impl<_Tp>* __a, _Tp __pattern, memory_order __order) _NOEXCEPT {
#if _LIBCPP_STD_VER >= 26
  if constexpr (is_scalar_v<_Tp>) {
    if consteval {
      _Tp __old      = __a->__a_value;
      __a->__a_value = static_cast<_Tp>(__old | __pattern);
      return __old;
    }
  }
#endif
  return __c11_atomic_fetch_or(
      std::addressof(__a->__a_value), __pattern, static_cast<__memory_order_underlying_t>(__order));
}

template <class _Tp>
_LIBCPP_HIDE_FROM_ABI _Tp
__cxx_atomic_fetch_xor(__cxx_atomic_base_impl<_Tp> volatile* __a, _Tp __pattern, memory_order __order) _NOEXCEPT {
  return __c11_atomic_fetch_xor(
      std::addressof(__a->__a_value), __pattern, static_cast<__memory_order_underlying_t>(__order));
}
template <class _Tp>
_LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp
__cxx_atomic_fetch_xor(__cxx_atomic_base_impl<_Tp>* __a, _Tp __pattern, memory_order __order) _NOEXCEPT {
#if _LIBCPP_STD_VER >= 26
  if constexpr (is_scalar_v<_Tp>) {
    if consteval {
      _Tp __old      = __a->__a_value;
      __a->__a_value = static_cast<_Tp>(__old ^ __pattern);
      return __old;
    }
  }
#endif
  return __c11_atomic_fetch_xor(
      std::addressof(__a->__a_value), __pattern, static_cast<__memory_order_underlying_t>(__order));
}

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___ATOMIC_SUPPORT_C11_H
