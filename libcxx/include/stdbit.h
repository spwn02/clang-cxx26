// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP_STDBIT_H
#define _LIBCPP_STDBIT_H

/*
    stdbit.h synopsis // since C++26

Macros:

    __STDC_VERSION_STDBIT_H__
    __STDC_ENDIAN_BIG__
    __STDC_ENDIAN_LITTLE__
    __STDC_ENDIAN_NATIVE__

Functions (each with _uc/_us/_ui/_ul/_ull overloads plus a generic
function template, all declared at global scope):

    stdc_leading_zeros
    stdc_leading_ones
    stdc_trailing_zeros
    stdc_trailing_ones
    stdc_first_leading_zero
    stdc_first_leading_one
    stdc_first_trailing_zero
    stdc_first_trailing_one
    stdc_count_zeros
    stdc_count_ones
    stdc_has_single_bit
    stdc_bit_width
    stdc_bit_floor
    stdc_bit_ceil

*/

#include <__config>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_STD_VER >= 26

#  include <__type_traits/integer_traits.h>
#  include <bit>
#  include <limits>

#  define __STDC_VERSION_STDBIT_H__ 202311L

#  define __STDC_ENDIAN_BIG__ __ORDER_BIG_ENDIAN__
#  define __STDC_ENDIAN_LITTLE__ __ORDER_LITTLE_ENDIAN__
#  define __STDC_ENDIAN_NATIVE__ __BYTE_ORDER__

template <std::__unsigned_integer _Tp>
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_leading_zeros(_Tp __value) {
  return std::countl_zero(__value);
}
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_leading_zeros_uc(unsigned char __value) {
  return stdc_leading_zeros(__value);
}
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_leading_zeros_us(unsigned short __value) {
  return stdc_leading_zeros(__value);
}
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_leading_zeros_ui(unsigned int __value) {
  return stdc_leading_zeros(__value);
}
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_leading_zeros_ul(unsigned long __value) {
  return stdc_leading_zeros(__value);
}
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_leading_zeros_ull(unsigned long long __value) {
  return stdc_leading_zeros(__value);
}

template <std::__unsigned_integer _Tp>
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_leading_ones(_Tp __value) {
  return std::countl_one(__value);
}
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_leading_ones_uc(unsigned char __value) {
  return stdc_leading_ones(__value);
}
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_leading_ones_us(unsigned short __value) {
  return stdc_leading_ones(__value);
}
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_leading_ones_ui(unsigned int __value) {
  return stdc_leading_ones(__value);
}
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_leading_ones_ul(unsigned long __value) {
  return stdc_leading_ones(__value);
}
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_leading_ones_ull(unsigned long long __value) {
  return stdc_leading_ones(__value);
}

template <std::__unsigned_integer _Tp>
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_trailing_zeros(_Tp __value) {
  return std::countr_zero(__value);
}
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_trailing_zeros_uc(unsigned char __value) {
  return stdc_trailing_zeros(__value);
}
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_trailing_zeros_us(unsigned short __value) {
  return stdc_trailing_zeros(__value);
}
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_trailing_zeros_ui(unsigned int __value) {
  return stdc_trailing_zeros(__value);
}
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_trailing_zeros_ul(unsigned long __value) {
  return stdc_trailing_zeros(__value);
}
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_trailing_zeros_ull(unsigned long long __value) {
  return stdc_trailing_zeros(__value);
}

template <std::__unsigned_integer _Tp>
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_trailing_ones(_Tp __value) {
  return std::countr_one(__value);
}
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_trailing_ones_uc(unsigned char __value) {
  return stdc_trailing_ones(__value);
}
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_trailing_ones_us(unsigned short __value) {
  return stdc_trailing_ones(__value);
}
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_trailing_ones_ui(unsigned int __value) {
  return stdc_trailing_ones(__value);
}
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_trailing_ones_ul(unsigned long __value) {
  return stdc_trailing_ones(__value);
}
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_trailing_ones_ull(unsigned long long __value) {
  return stdc_trailing_ones(__value);
}

// Index (counting from the most-significant bit, 1-based) of the first zero
// bit, or 0 if __value has no zero bits.
template <std::__unsigned_integer _Tp>
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_first_leading_zero(_Tp __value) {
  return __value == _Tp(-1) ? 0u : 1u + std::countl_one(__value);
}
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_first_leading_zero_uc(unsigned char __value) {
  return stdc_first_leading_zero(__value);
}
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_first_leading_zero_us(unsigned short __value) {
  return stdc_first_leading_zero(__value);
}
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_first_leading_zero_ui(unsigned int __value) {
  return stdc_first_leading_zero(__value);
}
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_first_leading_zero_ul(unsigned long __value) {
  return stdc_first_leading_zero(__value);
}
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_first_leading_zero_ull(unsigned long long __value) {
  return stdc_first_leading_zero(__value);
}

// Index (counting from the most-significant bit, 1-based) of the first one
// bit, or 0 if __value is zero.
template <std::__unsigned_integer _Tp>
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_first_leading_one(_Tp __value) {
  return __value == 0 ? 0u : 1u + std::countl_zero(__value);
}
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_first_leading_one_uc(unsigned char __value) {
  return stdc_first_leading_one(__value);
}
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_first_leading_one_us(unsigned short __value) {
  return stdc_first_leading_one(__value);
}
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_first_leading_one_ui(unsigned int __value) {
  return stdc_first_leading_one(__value);
}
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_first_leading_one_ul(unsigned long __value) {
  return stdc_first_leading_one(__value);
}
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_first_leading_one_ull(unsigned long long __value) {
  return stdc_first_leading_one(__value);
}

// Index (counting from the least-significant bit, 1-based) of the first zero
// bit, or 0 if __value has no zero bits.
template <std::__unsigned_integer _Tp>
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_first_trailing_zero(_Tp __value) {
  return __value == _Tp(-1) ? 0u : 1u + std::countr_one(__value);
}
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_first_trailing_zero_uc(unsigned char __value) {
  return stdc_first_trailing_zero(__value);
}
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_first_trailing_zero_us(unsigned short __value) {
  return stdc_first_trailing_zero(__value);
}
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_first_trailing_zero_ui(unsigned int __value) {
  return stdc_first_trailing_zero(__value);
}
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_first_trailing_zero_ul(unsigned long __value) {
  return stdc_first_trailing_zero(__value);
}
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_first_trailing_zero_ull(unsigned long long __value) {
  return stdc_first_trailing_zero(__value);
}

// Index (counting from the least-significant bit, 1-based) of the first one
// bit, or 0 if __value is zero.
template <std::__unsigned_integer _Tp>
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_first_trailing_one(_Tp __value) {
  return __value == 0 ? 0u : 1u + std::countr_zero(__value);
}
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_first_trailing_one_uc(unsigned char __value) {
  return stdc_first_trailing_one(__value);
}
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_first_trailing_one_us(unsigned short __value) {
  return stdc_first_trailing_one(__value);
}
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_first_trailing_one_ui(unsigned int __value) {
  return stdc_first_trailing_one(__value);
}
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_first_trailing_one_ul(unsigned long __value) {
  return stdc_first_trailing_one(__value);
}
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_first_trailing_one_ull(unsigned long long __value) {
  return stdc_first_trailing_one(__value);
}

template <std::__unsigned_integer _Tp>
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_count_zeros(_Tp __value) {
  return std::popcount(_Tp(~__value));
}
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_count_zeros_uc(unsigned char __value) {
  return stdc_count_zeros(__value);
}
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_count_zeros_us(unsigned short __value) {
  return stdc_count_zeros(__value);
}
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_count_zeros_ui(unsigned int __value) {
  return stdc_count_zeros(__value);
}
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_count_zeros_ul(unsigned long __value) {
  return stdc_count_zeros(__value);
}
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_count_zeros_ull(unsigned long long __value) {
  return stdc_count_zeros(__value);
}

template <std::__unsigned_integer _Tp>
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_count_ones(_Tp __value) {
  return std::popcount(__value);
}
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_count_ones_uc(unsigned char __value) {
  return stdc_count_ones(__value);
}
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_count_ones_us(unsigned short __value) {
  return stdc_count_ones(__value);
}
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_count_ones_ui(unsigned int __value) {
  return stdc_count_ones(__value);
}
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_count_ones_ul(unsigned long __value) {
  return stdc_count_ones(__value);
}
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_count_ones_ull(unsigned long long __value) {
  return stdc_count_ones(__value);
}

template <std::__unsigned_integer _Tp>
_LIBCPP_HIDE_FROM_ABI inline bool stdc_has_single_bit(_Tp __value) {
  return std::has_single_bit(__value);
}
_LIBCPP_HIDE_FROM_ABI inline bool stdc_has_single_bit_uc(unsigned char __value) {
  return stdc_has_single_bit(__value);
}
_LIBCPP_HIDE_FROM_ABI inline bool stdc_has_single_bit_us(unsigned short __value) {
  return stdc_has_single_bit(__value);
}
_LIBCPP_HIDE_FROM_ABI inline bool stdc_has_single_bit_ui(unsigned int __value) {
  return stdc_has_single_bit(__value);
}
_LIBCPP_HIDE_FROM_ABI inline bool stdc_has_single_bit_ul(unsigned long __value) {
  return stdc_has_single_bit(__value);
}
_LIBCPP_HIDE_FROM_ABI inline bool stdc_has_single_bit_ull(unsigned long long __value) {
  return stdc_has_single_bit(__value);
}

template <std::__unsigned_integer _Tp>
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_bit_width(_Tp __value) {
  return std::bit_width(__value);
}
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_bit_width_uc(unsigned char __value) { return stdc_bit_width(__value); }
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_bit_width_us(unsigned short __value) {
  return stdc_bit_width(__value);
}
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_bit_width_ui(unsigned int __value) { return stdc_bit_width(__value); }
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_bit_width_ul(unsigned long __value) { return stdc_bit_width(__value); }
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_bit_width_ull(unsigned long long __value) {
  return stdc_bit_width(__value);
}

template <std::__unsigned_integer _Tp>
_LIBCPP_HIDE_FROM_ABI inline _Tp stdc_bit_floor(_Tp __value) {
  return std::bit_floor(__value);
}
_LIBCPP_HIDE_FROM_ABI inline unsigned char stdc_bit_floor_uc(unsigned char __value) { return stdc_bit_floor(__value); }
_LIBCPP_HIDE_FROM_ABI inline unsigned short stdc_bit_floor_us(unsigned short __value) {
  return stdc_bit_floor(__value);
}
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_bit_floor_ui(unsigned int __value) { return stdc_bit_floor(__value); }
_LIBCPP_HIDE_FROM_ABI inline unsigned long stdc_bit_floor_ul(unsigned long __value) { return stdc_bit_floor(__value); }
_LIBCPP_HIDE_FROM_ABI inline unsigned long long stdc_bit_floor_ull(unsigned long long __value) {
  return stdc_bit_floor(__value);
}

// Unlike std::bit_ceil, stdc_bit_ceil is defined to return zero (rather than
// have undefined behavior) for values not representable in the return type.
template <std::__unsigned_integer _Tp>
_LIBCPP_HIDE_FROM_ABI inline _Tp stdc_bit_ceil(_Tp __value) {
  // __msb is itself representable (it's the largest power of two that fits
  // in _Tp), so the cutoff for "not representable" is strictly greater than
  // __msb, not merely having its top bit set.
  constexpr _Tp __msb = _Tp(1) << (std::numeric_limits<_Tp>::digits - 1);
  return (__value > __msb) ? 0 : std::bit_ceil(__value);
}
_LIBCPP_HIDE_FROM_ABI inline unsigned char stdc_bit_ceil_uc(unsigned char __value) { return stdc_bit_ceil(__value); }
_LIBCPP_HIDE_FROM_ABI inline unsigned short stdc_bit_ceil_us(unsigned short __value) {
  return stdc_bit_ceil(__value);
}
_LIBCPP_HIDE_FROM_ABI inline unsigned int stdc_bit_ceil_ui(unsigned int __value) { return stdc_bit_ceil(__value); }
_LIBCPP_HIDE_FROM_ABI inline unsigned long stdc_bit_ceil_ul(unsigned long __value) { return stdc_bit_ceil(__value); }
_LIBCPP_HIDE_FROM_ABI inline unsigned long long stdc_bit_ceil_ull(unsigned long long __value) {
  return stdc_bit_ceil(__value);
}

#endif // _LIBCPP_STD_VER >= 26

#endif // _LIBCPP_STDBIT_H
