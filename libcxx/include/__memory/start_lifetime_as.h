// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___MEMORY_START_LIFETIME_AS_H
#define _LIBCPP___MEMORY_START_LIFETIME_AS_H

#include <__assert>
#include <__config>
#include <__cstddef/size_t.h>
#include <__type_traits/is_implicit_lifetime.h>
#include <cstdint>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 23

// [obj.lifetime]: neither start_lifetime_as nor start_lifetime_as_array is
// specified as constexpr -- they implicitly create objects without running
// any code, which a compile-time evaluator has no way to observe absent
// dedicated support; a plain pointer cast is exactly what a call to either
// function compiles down to at runtime, matching the "Effects" wording's own
// "except that the storage is not accessed" clause (no bytes are read or
// written -- the storage already holds the created object's representation).
template <class _Tp>
_LIBCPP_HIDE_FROM_ABI _Tp* __start_lifetime_as_impl(void* __p) noexcept {
  static_assert(is_implicit_lifetime_v<_Tp>,
                "std::start_lifetime_as<T>(p) requires T to be an implicit-lifetime type");
  _LIBCPP_ASSERT_ARGUMENT_WITHIN_DOMAIN(__p != nullptr, "start_lifetime_as called with a null pointer");
  _LIBCPP_ASSERT_ARGUMENT_WITHIN_DOMAIN(
      reinterpret_cast<uintptr_t>(__p) % alignof(_Tp) == 0, "start_lifetime_as called with a misaligned pointer");
  return static_cast<_Tp*>(__p);
}

template <class _Tp>
_LIBCPP_HIDE_FROM_ABI _Tp* start_lifetime_as(void* __p) noexcept {
  return std::__start_lifetime_as_impl<_Tp>(__p);
}
template <class _Tp>
_LIBCPP_HIDE_FROM_ABI const _Tp* start_lifetime_as(const void* __p) noexcept {
  return std::__start_lifetime_as_impl<_Tp>(const_cast<void*>(__p));
}
template <class _Tp>
_LIBCPP_HIDE_FROM_ABI volatile _Tp* start_lifetime_as(volatile void* __p) noexcept {
  return std::__start_lifetime_as_impl<_Tp>(const_cast<void*>(__p));
}
template <class _Tp>
_LIBCPP_HIDE_FROM_ABI const volatile _Tp* start_lifetime_as(const volatile void* __p) noexcept {
  return std::__start_lifetime_as_impl<_Tp>(const_cast<void*>(__p));
}

// [obj.lifetime]: unlike start_lifetime_as, T need not itself be an
// implicit-lifetime type here -- array types (of any element type) are
// unconditionally implicit-lifetime per [basic.types.general], so the
// "array of n T" this is specified as equivalent to always already
// satisfies start_lifetime_as's own Mandates; only T's completeness is
// separately required.
template <class _Tp>
_LIBCPP_HIDE_FROM_ABI _Tp* __start_lifetime_as_array_impl(void* __p, size_t __n) noexcept {
  static_assert(sizeof(_Tp) >= 0, "std::start_lifetime_as_array<T> requires T to be a complete type");
  _LIBCPP_ASSERT_ARGUMENT_WITHIN_DOMAIN(
      __n <= size_t(-1) / sizeof(_Tp), "start_lifetime_as_array called with n too large");
  if (__n == 0)
    return static_cast<_Tp*>(__p);
  _LIBCPP_ASSERT_ARGUMENT_WITHIN_DOMAIN(__p != nullptr, "start_lifetime_as_array called with a null pointer and n > 0");
  _LIBCPP_ASSERT_ARGUMENT_WITHIN_DOMAIN(
      reinterpret_cast<uintptr_t>(__p) % alignof(_Tp) == 0,
      "start_lifetime_as_array called with a misaligned pointer");
  return static_cast<_Tp*>(__p);
}

template <class _Tp>
_LIBCPP_HIDE_FROM_ABI _Tp* start_lifetime_as_array(void* __p, size_t __n) noexcept {
  return std::__start_lifetime_as_array_impl<_Tp>(__p, __n);
}
template <class _Tp>
_LIBCPP_HIDE_FROM_ABI const _Tp* start_lifetime_as_array(const void* __p, size_t __n) noexcept {
  return std::__start_lifetime_as_array_impl<_Tp>(const_cast<void*>(__p), __n);
}
template <class _Tp>
_LIBCPP_HIDE_FROM_ABI volatile _Tp* start_lifetime_as_array(volatile void* __p, size_t __n) noexcept {
  return std::__start_lifetime_as_array_impl<_Tp>(const_cast<void*>(__p), __n);
}
template <class _Tp>
_LIBCPP_HIDE_FROM_ABI const volatile _Tp* start_lifetime_as_array(const volatile void* __p, size_t __n) noexcept {
  return std::__start_lifetime_as_array_impl<_Tp>(const_cast<void*>(__p), __n);
}

#endif // _LIBCPP_STD_VER >= 23

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___MEMORY_START_LIFETIME_AS_H
