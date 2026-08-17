// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___SIMD_ABI_H
#define _LIBCPP___SIMD_ABI_H

#include <__config>
#include <__cstddef/size_t.h>
#include <__simd/vectorizable.h>
#include <__type_traits/conditional.h>
#include <__type_traits/integral_constant.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

namespace simd {

// [simd.expos.abi]
//
// An ABI tag indicates a choice of size and binary representation. The portable tag below encodes
// the width only; the element type is supplied separately by basic_vec/basic_mask. That split is
// what lets basic_mask<Bytes, Abi> and basic_vec<T, Abi> agree on a width without the mask knowing
// the element type, as [simd.mask.overview] requires.
//
// Optimized backends do not need new tags: they key their storage and operations off the
// (element type, width) pair, so the ABI stays stable across instruction sets.
template <__simd_size_type _Np>
struct __simd_abi_fixed {};

// [simd.expos.abi]/4: when deduce-abi-t does not name an ABI tag type, it names an unspecified
// type. This is that type. It is deliberately never an ABI tag, so basic_vec/basic_mask over it
// are disabled specializations rather than hard errors.
struct __simd_abi_invalid {};

// [simd.expos.abi]/4: "The implementation-defined maximum for N is not smaller than 64."
inline constexpr __simd_size_type __simd_max_width = 64;

template <class _Abi>
inline constexpr bool __is_abi_tag = false;

template <__simd_size_type _Np>
inline constexpr bool __is_abi_tag<__simd_abi_fixed<_Np>> = _Np > 0 && _Np <= __simd_max_width;

template <class _Abi>
inline constexpr __simd_size_type __abi_width = 0;

template <__simd_size_type _Np>
inline constexpr __simd_size_type __abi_width<__simd_abi_fixed<_Np>> = _Np;

// [simd.expos.abi]/4
template <class _Tp, __simd_size_type _Np>
using __deduce_abi_t _LIBCPP_NODEBUG =
    __conditional_t<__vectorizable<_Tp> && (_Np > 0) && (_Np <= __simd_max_width),
                    __simd_abi_fixed<_Np>,
                    __simd_abi_invalid>;

// [simd.overview]/1.1 and [simd.mask.overview]/1
template <class _Tp, class _Abi>
inline constexpr bool __vec_enabled = __vectorizable<_Tp> && __is_abi_tag<_Abi>;

template <size_t _Bytes, class _Abi>
inline constexpr bool __mask_enabled = __valid_mask_element_size<_Bytes> && __is_abi_tag<_Abi>;

// [simd.expos.defn]/3-4
template <class _Tp, class _Abi>
inline constexpr __simd_size_type __simd_size_v = __vec_enabled<_Tp, _Abi> ? __abi_width<_Abi> : 0;

template <size_t _Bytes, class _Abi>
inline constexpr __simd_size_type __mask_size_v = __mask_enabled<_Bytes, _Abi> ? __abi_width<_Abi> : 0;

// [simd.expos.abi]/6: native-abi<T> is implementation-defined; the intent is the tag giving the
// most efficient execution for T on the current target.
//
// The baseline is a 128-bit register: N = max(1, 16 / sizeof(T)). For complex<double> (16 bytes)
// that is a width of 1, which is legal. Widening under __AVX__/__AVX512F__ belongs here, not at
// the use sites.
inline constexpr size_t __native_register_bytes = 16;

template <class _Tp>
struct __native_width : integral_constant<__simd_size_type, 0> {};

template <class _Tp>
  requires __vectorizable<_Tp>
struct __native_width<_Tp>
    : integral_constant<__simd_size_type,
                        sizeof(_Tp) >= __native_register_bytes
                            ? __simd_size_type{1}
                            : static_cast<__simd_size_type>(__native_register_bytes / sizeof(_Tp))> {};

template <class _Tp>
inline constexpr __simd_size_type __native_width_v = __native_width<_Tp>::value;

template <class _Tp>
using __native_abi _LIBCPP_NODEBUG = __deduce_abi_t<_Tp, __native_width_v<_Tp>>;

// [simd.syn]
template <size_t _Bytes, class _Abi>
class basic_mask;

template <class _Tp, class _Abi = __native_abi<_Tp>>
class basic_vec;

template <class _Tp, __simd_size_type _Np = __simd_size_v<_Tp, __native_abi<_Tp>>>
using vec = basic_vec<_Tp, __deduce_abi_t<_Tp, _Np>>;

} // namespace simd

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___SIMD_ABI_H
