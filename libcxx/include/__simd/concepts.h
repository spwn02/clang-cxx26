// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___SIMD_CONCEPTS_H
#define _LIBCPP___SIMD_CONCEPTS_H

#include <__concepts/arithmetic.h>
#include <__concepts/same_as.h>
#include <__config>
#include <__cstddef/size_t.h>
#include <__fwd/complex.h>
#include <__simd/abi.h>
#include <__simd/vectorizable.h>
#include <__type_traits/is_arithmetic.h>
#include <__type_traits/is_constructible.h>
#include <__type_traits/is_same.h>
#include <__type_traits/is_signed.h>
#include <__utility/declval.h>
#include <limits>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

namespace simd {

// [simd.expos.defn]/5: mask-element-size<basic_mask<Bytes, Abi>> has the value Bytes.
template <class _Tp>
inline constexpr size_t __mask_element_size = 0;

template <size_t _Bytes, class _Abi>
inline constexpr size_t __mask_element_size<basic_mask<_Bytes, _Abi>> = _Bytes;

// [simd.expos]
template <class _From, class _To>
concept __explicitly_convertible_to = requires { static_cast<_To>(std::declval<_From>()); };

template <class _Vp>
concept __simd_vec_type =
    same_as<_Vp, basic_vec<typename _Vp::value_type, typename _Vp::abi_type>> && is_default_constructible_v<_Vp>;

template <class _Vp>
concept __simd_mask_type =
    same_as<_Vp, basic_mask<__mask_element_size<_Vp>, typename _Vp::abi_type>> && is_default_constructible_v<_Vp>;

template <class _Vp>
concept __simd_floating_point = __simd_vec_type<_Vp> && floating_point<typename _Vp::value_type>;

template <class _Vp>
concept __simd_integral = __simd_vec_type<_Vp> && integral<typename _Vp::value_type>;

template <class _Vp>
using __simd_complex_value_type = typename _Vp::value_type::value_type;

template <class _Vp>
concept __simd_complex =
    __simd_vec_type<_Vp> && requires { typename _Vp::value_type::value_type; } &&
    same_as<typename _Vp::value_type, complex<__simd_complex_value_type<_Vp>>>;

// [simd.expos.defn]/6-7: deduced-vec-t<T> is an alias for decltype(x + x), where x is an lvalue of
// type const T.
//
// This is the mechanism behind the mixed vec/scalar math overloads. In
//   pow(const V& x, const deduced-vec-t<V>& y)
// the second parameter is a non-deduced context, so V is deduced from x alone and a scalar y is
// admitted through basic_vec's implicit broadcast constructor. That is why the broadcast
// constructor must stay implicit, and why its value-preserving constraint is load-bearing.
template <class _Tp>
using __deduced_vec_t = decltype(std::declval<const _Tp&>() + std::declval<const _Tp&>());

template <class _Tp>
concept __math_floating_point = requires { typename __deduced_vec_t<_Tp>; } && __simd_floating_point<__deduced_vec_t<_Tp>>;

// [simd.expos.defn]/7-8
template <class _BinaryOperation, class _Tp>
concept __reduction_binary_operation = requires(const _BinaryOperation __binary_op, const vec<_Tp, 1> __v) {
  { __binary_op(__v, __v) } -> same_as<vec<_Tp, 1>>;
};

// [simd.general]/8: the conversion from an arithmetic type U to a vectorizable type T is
// value-preserving if all possible values of U can be represented with type T.
template <class _From, class _To>
inline constexpr bool __value_preserving_conversion = false;

template <class _From, class _To>
  requires is_arithmetic_v<_From> && is_arithmetic_v<_To>
inline constexpr bool __value_preserving_conversion<_From, _To> = [] consteval {
  using _FL = numeric_limits<_From>;
  using _TL = numeric_limits<_To>;
  if constexpr (_FL::is_integer && _TL::is_integer) {
    // digits excludes the sign bit, so this covers signed->signed, unsigned->unsigned and
    // unsigned->signed uniformly; signed->unsigned is never value-preserving.
    return (!_FL::is_signed || _TL::is_signed) && _FL::digits <= _TL::digits;
  } else if constexpr (_FL::is_integer) {
    // Integer to floating point: representable when the mantissa is wide enough.
    return _FL::digits <= _TL::digits;
  } else if constexpr (_TL::is_integer) {
    return false; // Floating point to integer never preserves all values.
  } else {
    return _FL::digits <= _TL::digits && _FL::max_exponent <= _TL::max_exponent &&
           _FL::min_exponent >= _TL::min_exponent;
  }
}();

// [simd.ctor]/6.2-6.3: the converting basic_vec constructor is explicit when the source has the
// greater conversion rank. Rank is not size -- long and long long are both 8 bytes on LP64 and
// have different ranks -- so this cannot be spelled with sizeof.
template <class _Tp>
inline constexpr int __integer_conversion_rank = -1;

// clang-format off
template <> inline constexpr int __integer_conversion_rank<bool>               = 0;
template <> inline constexpr int __integer_conversion_rank<char>               = 1;
template <> inline constexpr int __integer_conversion_rank<signed char>        = 1;
template <> inline constexpr int __integer_conversion_rank<unsigned char>      = 1;
template <> inline constexpr int __integer_conversion_rank<short>              = 2;
template <> inline constexpr int __integer_conversion_rank<unsigned short>     = 2;
template <> inline constexpr int __integer_conversion_rank<int>                = 3;
template <> inline constexpr int __integer_conversion_rank<unsigned int>       = 3;
template <> inline constexpr int __integer_conversion_rank<long>               = 4;
template <> inline constexpr int __integer_conversion_rank<unsigned long>      = 4;
template <> inline constexpr int __integer_conversion_rank<long long>          = 5;
template <> inline constexpr int __integer_conversion_rank<unsigned long long> = 5;
// clang-format on

// char8_t/char16_t/char32_t/wchar_t rank with their underlying types.
template <>
inline constexpr int __integer_conversion_rank<char8_t> = 1;
template <>
inline constexpr int __integer_conversion_rank<char16_t> = 2;
template <>
inline constexpr int __integer_conversion_rank<char32_t> = 3;
#  if _LIBCPP_HAS_WIDE_CHARACTERS
template <>
inline constexpr int __integer_conversion_rank<wchar_t> = 3;
#  endif

template <class _Tp>
inline constexpr int __floating_point_conversion_rank = -1;

template <>
inline constexpr int __floating_point_conversion_rank<float> = 1;
template <>
inline constexpr int __floating_point_conversion_rank<double> = 2;
template <>
inline constexpr int __floating_point_conversion_rank<long double> = 3;

} // namespace simd

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___SIMD_CONCEPTS_H
