// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___BIT_BIT_SHIFT_H
#define _LIBCPP___BIT_BIT_SHIFT_H

#include <__config>
#include <__type_traits/integer_traits.h>
#include <__type_traits/make_unsigned.h>
#include <limits>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

// [bit.shift]: the result is x * 2^s (shl) or x * 2^-s (shr) rounded towards negative infinity, converted to T as if by
// static_cast<T>; a negative shift count shifts in the other direction.

// x * 2^n for n >= 0, wrapped to T
template <class _Tp>
_LIBCPP_HIDE_FROM_ABI constexpr _Tp __shift_up(_Tp __x, unsigned long long __n) noexcept {
  using _Up = make_unsigned_t<_Tp>;
  if (__n >= static_cast<unsigned long long>(numeric_limits<_Up>::digits))
    return 0;
  return static_cast<_Tp>(static_cast<_Up>(static_cast<_Up>(__x) << static_cast<int>(__n)));
}

// floor(x / 2^n) for n >= 0
template <class _Tp>
_LIBCPP_HIDE_FROM_ABI constexpr _Tp __shift_down(_Tp __x, unsigned long long __n) noexcept {
  constexpr unsigned long long __width = static_cast<unsigned long long>(numeric_limits<make_unsigned_t<_Tp>>::digits);
  if (__n >= __width) {
    if constexpr (__unsigned_integer<_Tp>)
      return 0;
    else
      return __x < 0 ? static_cast<_Tp>(-1) : static_cast<_Tp>(0);
  }
  return static_cast<_Tp>(__x >> static_cast<int>(__n));
}

// the magnitude of s (saturated to the range of unsigned long long: a larger count shifts everything out anyway), without
// overflowing for the minimum of a signed type
template <class _Sp>
_LIBCPP_HIDE_FROM_ABI constexpr unsigned long long __shift_magnitude(_Sp __s) noexcept {
  using _Us = make_unsigned_t<_Sp>;
  _Us __magnitude;
  if constexpr (__unsigned_integer<_Sp>) {
    __magnitude = __s;
  } else {
    __magnitude = __s < 0 ? static_cast<_Us>(_Us(0) - static_cast<_Us>(__s)) : static_cast<_Us>(__s);
  }
  constexpr unsigned long long __max = numeric_limits<unsigned long long>::max();
  if constexpr (numeric_limits<_Us>::digits > numeric_limits<unsigned long long>::digits) {
    if (__magnitude > static_cast<_Us>(__max))
      return __max;
  }
  return static_cast<unsigned long long>(__magnitude);
}

template <class _Sp>
_LIBCPP_HIDE_FROM_ABI constexpr bool __shift_is_negative(_Sp __s) noexcept {
  if constexpr (__unsigned_integer<_Sp>) {
    return false;
  } else {
    return __s < 0;
  }
}

template <class _Tp, class _Sp>
  requires((__signed_integer<_Tp> || __unsigned_integer<_Tp>) && (__signed_integer<_Sp> || __unsigned_integer<_Sp>))
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr _Tp shl(_Tp __x, _Sp __s) noexcept {
  const unsigned long long __n = std::__shift_magnitude(__s);
  return std::__shift_is_negative(__s) ? std::__shift_down(__x, __n) : std::__shift_up(__x, __n);
}

template <class _Tp, class _Sp>
  requires((__signed_integer<_Tp> || __unsigned_integer<_Tp>) && (__signed_integer<_Sp> || __unsigned_integer<_Sp>))
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr _Tp shr(_Tp __x, _Sp __s) noexcept {
  const unsigned long long __n = std::__shift_magnitude(__s);
  return std::__shift_is_negative(__s) ? std::__shift_up(__x, __n) : std::__shift_down(__x, __n);
}

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___BIT_BIT_SHIFT_H
