// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___SIMD_TRAITS_H
#define _LIBCPP___SIMD_TRAITS_H

#include <__bit/bit_ceil.h>
#include <__config>
#include <__cstddef/size_t.h>
#include <__simd/abi.h>
#include <__simd/concepts.h>
#include <__simd/vectorizable.h>
#include <__type_traits/integral_constant.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

namespace simd {

// [simd.traits]/1-2: alignment<T, U> has a member `value` if and only if T is a specialization of
// basic_vec and U is a vectorizable type. The value itself is an unspecified N identifying the
// alignment restriction for (converting) loads and stores of T over arrays of U.
//
// The primary template is deliberately empty: the absence of `value` is observable and is what
// makes alignment_v SFINAE-friendly.
template <class _Tp, class _Up = typename _Tp::value_type>
struct alignment {};

template <class _Tp, class _Abi, class _Up>
  requires __vec_enabled<_Tp, _Abi> && __vectorizable<_Up>
struct alignment<basic_vec<_Tp, _Abi>, _Up>
    : integral_constant<size_t, std::bit_ceil(sizeof(_Up) * static_cast<size_t>(__simd_size_v<_Tp, _Abi>))> {};

template <class _Tp, class _Up = typename _Tp::value_type>
inline constexpr size_t alignment_v = alignment<_Tp, _Up>::value;

// [simd.traits]/3-5: rebind<T, V>::type is present if and only if V is a data-parallel type, T is a
// vectorizable type, and deduce-abi-t<T, V::size()> names an ABI tag type.
template <class _Tp, class _Vp>
struct rebind {};

template <class _Tp, class _Up, class _Abi>
  requires __vectorizable<_Tp> && __vec_enabled<_Up, _Abi> &&
           __is_abi_tag<__deduce_abi_t<_Tp, __simd_size_v<_Up, _Abi>>>
struct rebind<_Tp, basic_vec<_Up, _Abi>> {
  using type _LIBCPP_NODEBUG = basic_vec<_Tp, __deduce_abi_t<_Tp, __simd_size_v<_Up, _Abi>>>;
};

template <class _Tp, size_t _Bytes, class _Abi>
  requires __vectorizable<_Tp> && __mask_enabled<_Bytes, _Abi> &&
           __is_abi_tag<__deduce_abi_t<_Tp, __mask_size_v<_Bytes, _Abi>>>
struct rebind<_Tp, basic_mask<_Bytes, _Abi>> {
  using type _LIBCPP_NODEBUG = basic_mask<sizeof(_Tp), __deduce_abi_t<_Tp, __mask_size_v<_Bytes, _Abi>>>;
};

template <class _Tp, class _Vp>
using rebind_t = typename rebind<_Tp, _Vp>::type;

// [simd.traits]/6-8: resize<N, V>::type is present if and only if V is a data-parallel type and
// some ABI tag yields width N for it.
template <__simd_size_type _Np, class _Vp>
struct resize {};

template <__simd_size_type _Np, class _Tp, class _Abi>
  requires __vec_enabled<_Tp, _Abi> && __is_abi_tag<__deduce_abi_t<_Tp, _Np>>
struct resize<_Np, basic_vec<_Tp, _Abi>> {
  using type _LIBCPP_NODEBUG = basic_vec<_Tp, __deduce_abi_t<_Tp, _Np>>;
};

template <__simd_size_type _Np, size_t _Bytes, class _Abi>
  requires __mask_enabled<_Bytes, _Abi> && (_Np > 0) && (_Np <= __simd_max_width)
struct resize<_Np, basic_mask<_Bytes, _Abi>> {
  using type _LIBCPP_NODEBUG = basic_mask<_Bytes, __simd_abi_fixed<_Np>>;
};

template <__simd_size_type _Np, class _Vp>
using resize_t = typename resize<_Np, _Vp>::type;

} // namespace simd

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___SIMD_TRAITS_H
