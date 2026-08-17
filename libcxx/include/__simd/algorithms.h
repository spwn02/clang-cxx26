// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___SIMD_ALGORITHMS_H
#define _LIBCPP___SIMD_ALGORITHMS_H

#include <__assert>
#include <__concepts/totally_ordered.h>
#include <__config>
#include <__cstddef/size_t.h>
#include <__simd/abi.h>
#include <__simd/basic_vec.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/pair.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

namespace simd {

// [simd.alg]
template <class _Tp, class _Abi>
  requires(__vec_enabled<_Tp, _Abi> && totally_ordered<_Tp>)
_LIBCPP_HIDE_FROM_ABI constexpr basic_vec<_Tp, _Abi> min(const basic_vec<_Tp, _Abi>& __a,
                                                          const basic_vec<_Tp, _Abi>& __b) noexcept {
  return basic_vec<_Tp, _Abi>([&](auto __i) { return __b[__i] < __a[__i] ? __b[__i] : __a[__i]; });
}

template <class _Tp, class _Abi>
  requires(__vec_enabled<_Tp, _Abi> && totally_ordered<_Tp>)
_LIBCPP_HIDE_FROM_ABI constexpr basic_vec<_Tp, _Abi> max(const basic_vec<_Tp, _Abi>& __a,
                                                          const basic_vec<_Tp, _Abi>& __b) noexcept {
  return basic_vec<_Tp, _Abi>([&](auto __i) { return __a[__i] < __b[__i] ? __b[__i] : __a[__i]; });
}

template <class _Tp, class _Abi>
  requires(__vec_enabled<_Tp, _Abi> && totally_ordered<_Tp>)
_LIBCPP_HIDE_FROM_ABI constexpr pair<basic_vec<_Tp, _Abi>, basic_vec<_Tp, _Abi>>
minmax(const basic_vec<_Tp, _Abi>& __a, const basic_vec<_Tp, _Abi>& __b) noexcept {
  return pair{min(__a, __b), max(__a, __b)};
}

// [simd.alg]/6-8: Preconditions -- no element in lo is greater than the corresponding element in
// hi. Not noexcept: the precondition check may be a diagnosed assertion.
template <class _Tp, class _Abi>
  requires(__vec_enabled<_Tp, _Abi> && totally_ordered<_Tp>)
_LIBCPP_HIDE_FROM_ABI constexpr basic_vec<_Tp, _Abi>
clamp(const basic_vec<_Tp, _Abi>& __v, const basic_vec<_Tp, _Abi>& __lo, const basic_vec<_Tp, _Abi>& __hi) {
  for (__simd_size_type __i = 0; __i != basic_vec<_Tp, _Abi>::size(); ++__i)
    _LIBCPP_ASSERT_UNCATEGORIZED(!(__hi[__i] < __lo[__i]), "simd::clamp: no element in lo may exceed the "
                                                            "corresponding element in hi");
  return min(max(__v, __lo), __hi);
}

// [simd.alg]/9: scalar overload.
template <class _Tp, class _Up>
_LIBCPP_HIDE_FROM_ABI constexpr auto select(bool __c, const _Tp& __a, const _Up& __b)
    -> remove_cvref_t<decltype(__c ? __a : __b)> {
  return __c ? __a : __b;
}

// [simd.alg]/10: found via ADL on __simd_select_impl, contrary to the usual lookup rules -- the
// hidden friends live inside basic_vec and basic_mask themselves ([simd.cond], [simd.mask.cond]).
template <size_t _Bytes, class _Abi, class _Tp, class _Up>
_LIBCPP_HIDE_FROM_ABI constexpr auto select(const basic_mask<_Bytes, _Abi>& __c, const _Tp& __a, const _Up& __b) noexcept
    -> decltype(__simd_select_impl(__c, __a, __b)) {
  return __simd_select_impl(__c, __a, __b);
}

} // namespace simd

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP___SIMD_ALGORITHMS_H
