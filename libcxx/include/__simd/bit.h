// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___SIMD_BIT_H
#define _LIBCPP___SIMD_BIT_H

#include <__bit/bit_ceil.h>
#include <__bit/bit_floor.h>
#include <__bit/bit_width.h>
#include <__bit/byteswap.h>
#include <__bit/countl.h>
#include <__bit/countr.h>
#include <__bit/has_single_bit.h>
#include <__bit/popcount.h>
#include <__bit/rotate.h>
#include <__concepts/arithmetic.h>
#include <__config>
#include <__simd/abi.h>
#include <__simd/basic_vec.h>
#include <__simd/concepts.h>
#include <__simd/traits.h>
#include <__type_traits/make_signed.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

namespace simd {

// [simd.bit]/1-2: byteswap is constrained on integral, not unsigned -- distinct from the rest of
// this clause, which requires an unsigned integer element type.
template <__simd_vec_type _Vp>
  requires integral<typename _Vp::value_type>
_LIBCPP_HIDE_FROM_ABI constexpr _Vp byteswap(const _Vp& __v) noexcept {
  return _Vp([&](auto __i) { return std::byteswap(__v[__i]); });
}

#  define _LIBCPP_SIMD_BIT_UNARY(_Name, ...)                                                                         \
    template <__simd_vec_type _Vp>                                                                                   \
      requires unsigned_integral<typename _Vp::value_type>                                                           \
    _LIBCPP_HIDE_FROM_ABI constexpr _Vp _Name(const _Vp& __v) __VA_ARGS__ {                                          \
      return _Vp([&](auto __i) { return std::_Name(__v[__i]); });                                                    \
    }

// [simd.bit]/3-5: bit_ceil has a Preconditions element (the result must be representable), so it is
// not noexcept; bit_floor is.
_LIBCPP_SIMD_BIT_UNARY(bit_ceil)
_LIBCPP_SIMD_BIT_UNARY(bit_floor, noexcept)

// [simd.bit]/9-10
template <__simd_vec_type _Vp>
  requires unsigned_integral<typename _Vp::value_type>
_LIBCPP_HIDE_FROM_ABI constexpr typename _Vp::mask_type has_single_bit(const _Vp& __v) noexcept {
  using _Mp = typename _Vp::mask_type;
  return _Mp([&](auto __i) -> bool { return std::has_single_bit(__v[__i]); });
}

// [simd.bit]/11-12: rotl/rotr by a per-lane count of a (possibly different) simd-vec-type V1, plus
// the by-scalar-int overload.
template <__simd_vec_type _V0, __simd_vec_type _V1>
  requires(unsigned_integral<typename _V0::value_type> && integral<typename _V1::value_type> &&
           _V0::size() == _V1::size() && sizeof(typename _V0::value_type) == sizeof(typename _V1::value_type))
_LIBCPP_HIDE_FROM_ABI constexpr _V0 rotl(const _V0& __v0, const _V1& __v1) noexcept {
  return _V0([&](auto __i) { return std::rotl(__v0[__i], static_cast<int>(__v1[__i])); });
}

template <__simd_vec_type _Vp>
  requires unsigned_integral<typename _Vp::value_type>
_LIBCPP_HIDE_FROM_ABI constexpr _Vp rotl(const _Vp& __v, int __s) noexcept {
  return _Vp([&](auto __i) { return std::rotl(__v[__i], __s); });
}

template <__simd_vec_type _V0, __simd_vec_type _V1>
  requires(unsigned_integral<typename _V0::value_type> && integral<typename _V1::value_type> &&
           _V0::size() == _V1::size() && sizeof(typename _V0::value_type) == sizeof(typename _V1::value_type))
_LIBCPP_HIDE_FROM_ABI constexpr _V0 rotr(const _V0& __v0, const _V1& __v1) noexcept {
  return _V0([&](auto __i) { return std::rotr(__v0[__i], static_cast<int>(__v1[__i])); });
}

template <__simd_vec_type _Vp>
  requires unsigned_integral<typename _Vp::value_type>
_LIBCPP_HIDE_FROM_ABI constexpr _Vp rotr(const _Vp& __v, int __s) noexcept {
  return _Vp([&](auto __i) { return std::rotr(__v[__i], __s); });
}

#  undef _LIBCPP_SIMD_BIT_UNARY

// [simd.bit]/15-16: bit_width, countl_zero, countl_one, countr_zero, countr_one, popcount all
// return rebind_t<make_signed_t<value_type>, V>.
#  define _LIBCPP_SIMD_BIT_COUNT(_Name)                                                                              \
    template <__simd_vec_type _Vp>                                                                                   \
      requires unsigned_integral<typename _Vp::value_type>                                                           \
    _LIBCPP_HIDE_FROM_ABI constexpr rebind_t<make_signed_t<typename _Vp::value_type>, _Vp> _Name(                    \
        const _Vp& __v) noexcept {                                                                                   \
      using _Sp = make_signed_t<typename _Vp::value_type>;                                                           \
      using _Rp = rebind_t<_Sp, _Vp>;                                                                                \
      return _Rp([&](auto __i) { return static_cast<_Sp>(std::_Name(__v[__i])); });                                  \
    }

_LIBCPP_SIMD_BIT_COUNT(bit_width)
_LIBCPP_SIMD_BIT_COUNT(countl_zero)
_LIBCPP_SIMD_BIT_COUNT(countl_one)
_LIBCPP_SIMD_BIT_COUNT(countr_zero)
_LIBCPP_SIMD_BIT_COUNT(countr_one)
_LIBCPP_SIMD_BIT_COUNT(popcount)

#  undef _LIBCPP_SIMD_BIT_COUNT

} // namespace simd

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___SIMD_BIT_H
