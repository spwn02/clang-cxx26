// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___SIMD_VECTORIZABLE_H
#define _LIBCPP___SIMD_VECTORIZABLE_H

#include <__config>
#include <__cstddef/ptrdiff_t.h>
#include <__cstddef/size_t.h>
#include <__fwd/complex.h>
#include <__type_traits/enable_if.h>
#include <__type_traits/integral_constant.h>
#include <__type_traits/is_same.h>
#include <__type_traits/remove_cv.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

namespace simd {

// [simd.expos.defn]/1: simd-size-type is an alias for a signed integer type.
using __simd_size_type = ptrdiff_t;

// [simd.general]/2: the set of vectorizable types comprises
//   - all standard integer types, character types, and the types float and double,
//   - std::float16_t, std::float32_t, and std::float64_t if defined, and
//   - complex<T> where T is a vectorizable floating-point type.
//
// Note that this set is *not* `is_integral_v || is_floating_point_v`: bool is excluded
// (it is a boolean type, not a standard integer type) and long double is excluded (it is
// a standard floating-point type but is deliberately absent from the list above).

template <class _Tp>
inline constexpr bool __vectorizable_floating_point = false;

template <>
inline constexpr bool __vectorizable_floating_point<float> = true;
template <>
inline constexpr bool __vectorizable_floating_point<double> = true;

// Extension point for the extended floating-point types. libc++ does not currently provide
// <stdfloat>, so std::float16_t and friends do not exist; when they do, enable them here and
// the whole of <simd> follows without further change.
#  if defined(__STDCPP_FLOAT16_T__) && __has_include(<stdfloat>)
template <>
inline constexpr bool __vectorizable_floating_point<_Float16> = true;
#  endif

template <class _Tp>
inline constexpr bool __vectorizable_integer = false;

// Standard integer types.
template <>
inline constexpr bool __vectorizable_integer<signed char> = true;
template <>
inline constexpr bool __vectorizable_integer<unsigned char> = true;
template <>
inline constexpr bool __vectorizable_integer<short> = true;
template <>
inline constexpr bool __vectorizable_integer<unsigned short> = true;
template <>
inline constexpr bool __vectorizable_integer<int> = true;
template <>
inline constexpr bool __vectorizable_integer<unsigned int> = true;
template <>
inline constexpr bool __vectorizable_integer<long> = true;
template <>
inline constexpr bool __vectorizable_integer<unsigned long> = true;
template <>
inline constexpr bool __vectorizable_integer<long long> = true;
template <>
inline constexpr bool __vectorizable_integer<unsigned long long> = true;

// Character types.
template <>
inline constexpr bool __vectorizable_integer<char> = true;
template <>
inline constexpr bool __vectorizable_integer<char8_t> = true;
template <>
inline constexpr bool __vectorizable_integer<char16_t> = true;
template <>
inline constexpr bool __vectorizable_integer<char32_t> = true;
#  if _LIBCPP_HAS_WIDE_CHARACTERS
template <>
inline constexpr bool __vectorizable_integer<wchar_t> = true;
#  endif

template <class _Tp>
inline constexpr bool __vectorizable_complex = false;

template <class _Tp>
inline constexpr bool __vectorizable_complex<complex<_Tp>> = __vectorizable_floating_point<_Tp>;

template <class _Tp>
inline constexpr bool __vectorizable =
    __vectorizable_integer<remove_cv_t<_Tp>> || __vectorizable_floating_point<remove_cv_t<_Tp>> ||
    __vectorizable_complex<remove_cv_t<_Tp>>;

// [simd.expos.defn]/2: integer-from<Bytes> is an alias for a signed integer type T such that
// sizeof(T) equals Bytes. The member is absent when no *vectorizable* signed integer type has
// that size, which is what makes basic_mask's operator+/-/~ deleted for such widths
// ([simd.mask.unary]/3).
template <size_t _Bytes>
struct __integer_from_impl {};

template <>
struct __integer_from_impl<1> {
  using type _LIBCPP_NODEBUG = signed char;
};
template <>
struct __integer_from_impl<2> {
  using type _LIBCPP_NODEBUG = short;
};
template <>
struct __integer_from_impl<4> {
  using type _LIBCPP_NODEBUG = int;
};
template <>
struct __integer_from_impl<8> {
  using type _LIBCPP_NODEBUG = long long;
};

template <size_t _Bytes>
using __integer_from = typename __integer_from_impl<_Bytes>::type;

template <size_t _Bytes>
inline constexpr bool __has_integer_from = requires { typename __integer_from_impl<_Bytes>::type; };

// The set of element sizes for which some vectorizable type exists. This is the predicate behind
// [simd.mask.overview]/1.1: basic_mask<Bytes, Abi> is disabled when no vectorizable type has
// sizeof equal to Bytes.
//
// 1, 2, 4, 8 come from the integer and floating-point types; 8 and 16 additionally come from
// complex<float> and complex<double>.
template <size_t _Bytes>
inline constexpr bool __valid_mask_element_size = _Bytes == 1 || _Bytes == 2 || _Bytes == 4 || _Bytes == 8 || _Bytes == 16;

} // namespace simd

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___SIMD_VECTORIZABLE_H
