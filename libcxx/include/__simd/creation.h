// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___SIMD_CREATION_H
#define _LIBCPP___SIMD_CREATION_H

#include <__config>
#include <__cstddef/size_t.h>
#include <__simd/abi.h>
#include <__simd/basic_mask.h>
#include <__simd/basic_vec.h>
#include <__simd/concepts.h>
#include <__simd/traits.h>
#include <__tuple/tuple_like.h>
#include <array>
#include <tuple>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

namespace simd {

template <class _Tp, size_t>
using __ignore_index_t = _Tp;

// [simd.creation]/1-3: chunk<T>(x) where T is the chunk type. When x.size() is not a multiple of
// T::size(), the Constraints require resize_t<x.size() % T::size(), T> to be valid, and the return
// becomes a tuple of N objects of type T plus one tail object of that resized type -- rather than
// the fixed-size array returned in the divisible case.
template <class _Tp, class _Xp>
_LIBCPP_HIDE_FROM_ABI constexpr auto __chunk_impl(const _Xp& __x) noexcept {
  constexpr auto __n_full = _Xp::size() / _Tp::size();
  constexpr auto __rem    = _Xp::size() % _Tp::size();
  if constexpr (__rem == 0) {
    array<_Tp, static_cast<size_t>(__n_full)> __result{};
    for (__simd_size_type __block = 0; __block != __n_full; ++__block)
      __result[static_cast<size_t>(__block)] = _Tp([&](auto __i) { return __x[__block * _Tp::size() + __i]; });
    return __result;
  } else {
    using _Tail = resize_t<__rem, _Tp>;
    return [&]<size_t... _Is>(index_sequence<_Is...>) {
      auto __piece = [&](__simd_size_type __block) {
        return _Tp([&](auto __i) { return __x[__block * _Tp::size() + __i]; });
      };
      return tuple<__ignore_index_t<_Tp, _Is>..., _Tail>{
          __piece(static_cast<__simd_size_type>(_Is))...,
          _Tail([&](auto __i) { return __x[__n_full * _Tp::size() + __i]; })};
    }(make_index_sequence<static_cast<size_t>(__n_full)>{});
  }
}

template <class _Tp, class _Abi>
  requires __vec_enabled<typename _Tp::value_type, _Abi> && requires { typename _Tp::value_type; }
_LIBCPP_HIDE_FROM_ABI constexpr auto chunk(const basic_vec<typename _Tp::value_type, _Abi>& __x) noexcept {
  return __chunk_impl<_Tp>(__x);
}

// _Tp::value_type is always bool for every basic_mask specialization, so it cannot be used to
// recover _Tp's Bytes the way the basic_vec overload above uses _Tp::value_type -- __mask_element_size
// ([simd.expos.defn]/5, "mask-element-size<basic_mask<Bytes, Abi>> has the value Bytes") is the
// correct source for that instead.
template <class _Tp, class _Abi>
  requires __mask_enabled<__mask_element_size<_Tp>, _Abi>
_LIBCPP_HIDE_FROM_ABI constexpr auto chunk(const basic_mask<__mask_element_size<_Tp>, _Abi>& __x) noexcept {
  return __chunk_impl<_Tp>(__x);
}

// [simd.creation]/4-5
template <__simd_size_type _Np, class _Tp, class _Abi>
_LIBCPP_HIDE_FROM_ABI constexpr auto chunk(const basic_vec<_Tp, _Abi>& __x) noexcept {
  return chunk<resize_t<_Np, basic_vec<_Tp, _Abi>>>(__x);
}

template <__simd_size_type _Np, size_t _Bytes, class _Abi>
_LIBCPP_HIDE_FROM_ABI constexpr auto chunk(const basic_mask<_Bytes, _Abi>& __x) noexcept {
  return chunk<resize_t<_Np, basic_mask<_Bytes, _Abi>>>(__x);
}

// [simd.creation]/6: cat.
template <class _Tp, class _FirstAbi, class... _RestAbis>
_LIBCPP_HIDE_FROM_ABI constexpr auto cat(const basic_vec<_Tp, _FirstAbi>& __first,
                                         const basic_vec<_Tp, _RestAbis>&... __rest) noexcept {
  constexpr __simd_size_type __total =
      basic_vec<_Tp, _FirstAbi>::size() + (__simd_size_type{0} + ... + basic_vec<_Tp, _RestAbis>::size());
  using _Result = resize_t<__total, basic_vec<_Tp, _FirstAbi>>;
  __simd_size_type __position = 0;
  _Tp __elements[static_cast<size_t>(__total)];
  auto __append = [&]<class _Abi>(const basic_vec<_Tp, _Abi>& __value) {
    for (__simd_size_type __i = 0; __i != basic_vec<_Tp, _Abi>::size(); ++__i)
      __elements[static_cast<size_t>(__position++)] = __value[__i];
  };
  __append(__first);
  (__append(__rest), ...);
  return _Result([&](auto __i) { return __elements[__i]; });
}

template <size_t _Bytes, class _FirstAbi, class... _RestAbis>
_LIBCPP_HIDE_FROM_ABI constexpr auto cat(const basic_mask<_Bytes, _FirstAbi>& __first,
                                         const basic_mask<_Bytes, _RestAbis>&... __rest) noexcept {
  constexpr __simd_size_type __total =
      basic_mask<_Bytes, _FirstAbi>::size() + (__simd_size_type{0} + ... + basic_mask<_Bytes, _RestAbis>::size());
  using _Result = resize_t<__total, basic_mask<_Bytes, _FirstAbi>>;
  __simd_size_type __position = 0;
  bool __elements[static_cast<size_t>(__total)];
  auto __append = [&]<class _Abi>(const basic_mask<_Bytes, _Abi>& __value) {
    for (__simd_size_type __i = 0; __i != basic_mask<_Bytes, _Abi>::size(); ++__i)
      __elements[static_cast<size_t>(__position++)] = __value[__i];
  };
  __append(__first);
  (__append(__rest), ...);
  return _Result([&](auto __i) -> bool { return __elements[__i]; });
}

} // namespace simd

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___SIMD_CREATION_H
