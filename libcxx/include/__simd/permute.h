// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___SIMD_PERMUTE_H
#define _LIBCPP___SIMD_PERMUTE_H

#include <__assert>
#include <__concepts/arithmetic.h>
#include <__config>
#include <__cstddef/size_t.h>
#include <__ranges/concepts.h>
#include <__ranges/data.h>
#include <__ranges/size.h>
#include <__simd/abi.h>
#include <__simd/basic_mask.h>
#include <__simd/basic_vec.h>
#include <__simd/concepts.h>
#include <__simd/flags.h>
#include <__simd/reductions.h>
#include <__simd/traits.h>
#include <__type_traits/conditional.h>
#include <__type_traits/is_void.h>
#include <__type_traits/type_identity.h>
#include <__utility/forward.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

namespace simd {

inline constexpr __simd_size_type zero_element   = -1;
inline constexpr __simd_size_type uninit_element = -2;

// [simd.permute.static]
//
// gen-fn(i) calls idxmap with a *plain* simd_size_type value (not integral_constant) -- confirmed
// by the Constraints wording itself: "invoke_result_t<IdxMap&, simd-size-type> ... satisfies
// integral". That same Constraints clause is also what must disambiguate this overload set from
// [simd.permute.dynamic]'s permute(v, indices): an index *vector* has no call operator at all, so
// requiring IdxMap to be callable this way correctly excludes it, leaving dynamic permute as the
// sole match for `permute(v, some_vec)`.
//
// KNOWN DEVIATION: the clause's Mandates additionally requires gen-fn(i) to be a constant
// expression whose value lies in {zero_element, uninit_element} union [0, V::size()) for every i,
// making an out-of-range index ill-formed, no diagnostic required, at compile time. This
// implementation evaluates gen-fn(i) as a constant expression when idxmap is itself
// constexpr-callable (the ordinary case), which already catches most violations as genuine compile
// errors; what it does NOT do is force compile-time evaluation for an idxmap that isn't
// constexpr-callable at all, nor diagnose exactly per the Mandates wording. What IS implemented
// precisely regardless: the three-way Returns semantics -- zero_element yields value_type(),
// uninit_element yields a valid-but-unspecified value, and any in-range value returns v[value]; an
// out-of-range value is caught by a diagnosed runtime assertion.
template <class _IdxMap>
concept __idx_map_1arg = requires(_IdxMap& __m, __simd_size_type __i) {
  { __m(__i) } -> integral;
};
template <class _IdxMap>
concept __idx_map_2arg = requires(_IdxMap& __m, __simd_size_type __i, __simd_size_type __n) {
  { __m(__i, __n) } -> integral;
};
template <class _IdxMap>
concept __idx_map_like = __idx_map_1arg<_IdxMap> || __idx_map_2arg<_IdxMap>;

template <class _IdxMap, __simd_size_type _Size>
_LIBCPP_HIDE_FROM_ABI constexpr __simd_size_type __permute_gen_fn(_IdxMap& __idxmap, __simd_size_type __i) {
  if constexpr (__idx_map_2arg<_IdxMap>)
    return static_cast<__simd_size_type>(__idxmap(__i, _Size));
  else
    return static_cast<__simd_size_type>(__idxmap(__i));
}

template <__simd_size_type _Np, __simd_vec_type _Vp, __idx_map_like _IdxMap>
_LIBCPP_HIDE_FROM_ABI constexpr resize_t<_Np, _Vp> permute(const _Vp& __v, _IdxMap&& __idxmap) {
  using _Result = resize_t<_Np, _Vp>;
  using _Tp     = typename _Vp::value_type;
  return _Result([&]<class _Ic>(_Ic) {
    const __simd_size_type __src_ix = __permute_gen_fn<_IdxMap, _Vp::size()>(__idxmap, _Ic::value);
    if (__src_ix == zero_element)
      return _Tp();
    if (__src_ix == uninit_element)
      return _Tp(); // "unspecified value": default is a conforming choice.
    _LIBCPP_ASSERT_VALID_INPUT_RANGE(__src_ix >= 0 && __src_ix < _Vp::size(), "simd::permute: index out of range");
    return __v[__src_ix];
  });
}

template <__simd_vec_type _Vp, __idx_map_like _IdxMap>
_LIBCPP_HIDE_FROM_ABI constexpr resize_t<_Vp::size(), _Vp> permute(const _Vp& __v, _IdxMap&& __idxmap) {
  return permute<_Vp::size()>(__v, std::forward<_IdxMap>(__idxmap));
}

template <__simd_size_type _Np, __simd_mask_type _Vp, __idx_map_like _IdxMap>
_LIBCPP_HIDE_FROM_ABI constexpr resize_t<_Np, _Vp> permute(const _Vp& __v, _IdxMap&& __idxmap) {
  using _Result = resize_t<_Np, _Vp>;
  return _Result([&]<class _Ic>(_Ic) -> bool {
    const __simd_size_type __src_ix = __permute_gen_fn<_IdxMap, _Vp::size()>(__idxmap, _Ic::value);
    if (__src_ix == zero_element)
      return false;
    if (__src_ix == uninit_element)
      return false;
    _LIBCPP_ASSERT_VALID_INPUT_RANGE(__src_ix >= 0 && __src_ix < _Vp::size(), "simd::permute: index out of range");
    return __v[__src_ix];
  });
}

template <__simd_mask_type _Vp, __idx_map_like _IdxMap>
_LIBCPP_HIDE_FROM_ABI constexpr resize_t<_Vp::size(), _Vp> permute(const _Vp& __v, _IdxMap&& __idxmap) {
  return permute<_Vp::size()>(__v, std::forward<_IdxMap>(__idxmap));
}

// [simd.permute.dynamic]
template <__simd_vec_type _Vp, __simd_integral _Ip>
_LIBCPP_HIDE_FROM_ABI constexpr resize_t<_Ip::size(), _Vp> permute(const _Vp& __v, const _Ip& __indices) {
  using _Result = resize_t<_Ip::size(), _Vp>;
  return _Result([&](auto __i) {
    const auto __ix = __indices[__i];
    _LIBCPP_ASSERT_VALID_INPUT_RANGE(__ix >= 0 && __ix < _Vp::size(), "simd::permute: index out of range");
    return __v[__ix];
  });
}

template <__simd_mask_type _Vp, __simd_integral _Ip>
_LIBCPP_HIDE_FROM_ABI constexpr resize_t<_Ip::size(), _Vp> permute(const _Vp& __v, const _Ip& __indices) {
  using _Result = resize_t<_Ip::size(), _Vp>;
  return _Result([&](auto __i) -> bool {
    const auto __ix = __indices[__i];
    _LIBCPP_ASSERT_VALID_INPUT_RANGE(__ix >= 0 && __ix < _Vp::size(), "simd::permute: index out of range");
    return __v[__ix];
  });
}

// [simd.permute.mask]: compress/expand.
template <class _Selector>
_LIBCPP_HIDE_FROM_ABI constexpr __simd_size_type __bit_index(const _Selector& __selector, __simd_size_type __rank) {
  __simd_size_type __count = 0;
  for (__simd_size_type __j = 0; __j != _Selector::size(); ++__j) {
    if (__selector[__j]) {
      if (__count == __rank)
        return __j;
      ++__count;
    }
  }
  return -1;
}

template <__simd_vec_type _Vp>
_LIBCPP_HIDE_FROM_ABI constexpr _Vp compress(const _Vp& __v, const typename _Vp::mask_type& __selector) {
  const auto __count = reduce_count(__selector);
  return _Vp([&](auto __i) {
    return __i < __count ? __v[__bit_index(__selector, __i)] : typename _Vp::value_type();
  });
}

template <__simd_mask_type _Vp>
_LIBCPP_HIDE_FROM_ABI constexpr _Vp compress(const _Vp& __v, const type_identity_t<_Vp>& __selector) {
  const auto __count = reduce_count(__selector);
  return _Vp([&](auto __i) -> bool { return __i < __count && __v[__bit_index(__selector, __i)]; });
}

template <__simd_vec_type _Vp>
_LIBCPP_HIDE_FROM_ABI constexpr _Vp
compress(const _Vp& __v, const typename _Vp::mask_type& __selector, const typename _Vp::value_type& __fill_value) {
  const auto __count = reduce_count(__selector);
  return _Vp([&](auto __i) { return __i < __count ? __v[__bit_index(__selector, __i)] : __fill_value; });
}

template <__simd_mask_type _Vp>
_LIBCPP_HIDE_FROM_ABI constexpr _Vp compress(
    const _Vp& __v, const type_identity_t<_Vp>& __selector, const typename _Vp::value_type& __fill_value) {
  const auto __count = reduce_count(__selector);
  return _Vp([&](auto __i) -> bool { return __i < __count ? __v[__bit_index(__selector, __i)] : __fill_value; });
}

template <class _Selector>
_LIBCPP_HIDE_FROM_ABI constexpr __simd_size_type __rank_of(const _Selector& __selector, __simd_size_type __b) {
  __simd_size_type __rank = 0;
  for (__simd_size_type __j = 0; __j != __b; ++__j)
    if (__selector[__j])
      ++__rank;
  return __rank;
}

template <__simd_vec_type _Vp>
_LIBCPP_HIDE_FROM_ABI constexpr _Vp expand(const _Vp& __v, const typename _Vp::mask_type& __selector, const _Vp& __original = {}) {
  return _Vp([&](auto __i) {
    return __selector[__i] ? __v[__rank_of(__selector, __i)] : __original[__i];
  });
}

template <__simd_mask_type _Vp>
_LIBCPP_HIDE_FROM_ABI constexpr _Vp
expand(const _Vp& __v, const type_identity_t<_Vp>& __selector, const _Vp& __original = {}) {
  return _Vp([&](auto __i) -> bool { return __selector[__i] ? __v[__rank_of(__selector, __i)] : __original[__i]; });
}

// [simd.permute.memory]: gather/scatter. unchecked_* is spelled as an unconditional delegation to
// partial_*, per the clause's own "Effects: Equivalent to" wording for the analogous load/store
// functions -- the "unchecked" contract only relaxes what the caller must guarantee, not what the
// implementation does.
template <class _Rp, class _Ip>
using __gather_default_vec _LIBCPP_NODEBUG = basic_vec<ranges::range_value_t<_Rp>, __deduce_abi_t<ranges::range_value_t<_Rp>, _Ip::size()>>;

template <class _Vp = void, ranges::contiguous_range _Rp, __simd_integral _Ip, class... _Flags>
  requires ranges::sized_range<_Rp>
_LIBCPP_HIDE_FROM_ABI constexpr auto partial_gather_from(_Rp&& __src_range, const typename _Ip::mask_type& __mask,
                                                         const _Ip& __indices, flags<_Flags...> = {}) {
  using _Result = conditional_t<is_void_v<_Vp>, __gather_default_vec<_Rp, _Ip>, _Vp>;
  using _Tp     = typename _Result::value_type;
  auto* __p                  = ranges::data(__src_range);
  const auto __in_size       = ranges::size(__src_range);
  return _Result([&](auto __i) {
    const auto __ix = __indices[__i];
    return (__mask[__i] && __ix >= 0 && static_cast<decltype(__in_size)>(__ix) < __in_size)
             ? static_cast<_Tp>(__p[__ix])
             : _Tp();
  });
}

template <class _Vp = void, ranges::contiguous_range _Rp, __simd_integral _Ip, class... _Flags>
  requires ranges::sized_range<_Rp>
_LIBCPP_HIDE_FROM_ABI constexpr auto partial_gather_from(_Rp&& __src_range, const _Ip& __indices, flags<_Flags...> __f = {}) {
  using _Result = conditional_t<is_void_v<_Vp>, __gather_default_vec<_Rp, _Ip>, _Vp>;
  return partial_gather_from<_Result>(std::forward<_Rp>(__src_range), typename _Result::mask_type(true), __indices, __f);
}

template <class _Vp = void, ranges::contiguous_range _Rp, __simd_integral _Ip, class... _Flags>
  requires ranges::sized_range<_Rp>
_LIBCPP_HIDE_FROM_ABI constexpr auto unchecked_gather_from(_Rp&& __src_range, const typename _Ip::mask_type& __mask,
                                                            const _Ip& __indices, flags<_Flags...> __f = {}) {
  using _Result = conditional_t<is_void_v<_Vp>, __gather_default_vec<_Rp, _Ip>, _Vp>;
  return partial_gather_from<_Result>(std::forward<_Rp>(__src_range), __mask, __indices, __f);
}

template <class _Vp = void, ranges::contiguous_range _Rp, __simd_integral _Ip, class... _Flags>
  requires ranges::sized_range<_Rp>
_LIBCPP_HIDE_FROM_ABI constexpr auto unchecked_gather_from(_Rp&& __src_range, const _Ip& __indices, flags<_Flags...> __f = {}) {
  using _Result = conditional_t<is_void_v<_Vp>, __gather_default_vec<_Rp, _Ip>, _Vp>;
  return partial_gather_from<_Result>(std::forward<_Rp>(__src_range), typename _Result::mask_type(true), __indices, __f);
}

template <__simd_vec_type _Vp, ranges::contiguous_range _Rp, __simd_integral _Ip, class... _Flags>
  requires ranges::sized_range<_Rp>
_LIBCPP_HIDE_FROM_ABI constexpr void
partial_scatter_to(const _Vp& __v, _Rp&& __dst_range, const typename _Ip::mask_type& __mask, const _Ip& __indices, flags<_Flags...> = {}) {
  auto* __p            = ranges::data(__dst_range);
  const auto __out_size = ranges::size(__dst_range);
  for (__simd_size_type __i = 0; __i != _Vp::size(); ++__i) {
    const auto __ix = __indices[__i];
    if (__mask[__i] && __ix >= 0 && static_cast<decltype(__out_size)>(__ix) < __out_size)
      __p[__ix] = __v[__i];
  }
}

template <__simd_vec_type _Vp, ranges::contiguous_range _Rp, __simd_integral _Ip, class... _Flags>
  requires ranges::sized_range<_Rp>
_LIBCPP_HIDE_FROM_ABI constexpr void
partial_scatter_to(const _Vp& __v, _Rp&& __dst_range, const _Ip& __indices, flags<_Flags...> __f = {}) {
  partial_scatter_to(__v, std::forward<_Rp>(__dst_range), typename _Ip::mask_type(true), __indices, __f);
}

template <__simd_vec_type _Vp, ranges::contiguous_range _Rp, __simd_integral _Ip, class... _Flags>
  requires ranges::sized_range<_Rp>
_LIBCPP_HIDE_FROM_ABI constexpr void unchecked_scatter_to(
    const _Vp& __v, _Rp&& __dst_range, const typename _Ip::mask_type& __mask, const _Ip& __indices, flags<_Flags...> __f = {}) {
  partial_scatter_to(__v, std::forward<_Rp>(__dst_range), __mask, __indices, __f);
}

template <__simd_vec_type _Vp, ranges::contiguous_range _Rp, __simd_integral _Ip, class... _Flags>
  requires ranges::sized_range<_Rp>
_LIBCPP_HIDE_FROM_ABI constexpr void
unchecked_scatter_to(const _Vp& __v, _Rp&& __dst_range, const _Ip& __indices, flags<_Flags...> __f = {}) {
  partial_scatter_to(__v, std::forward<_Rp>(__dst_range), typename _Ip::mask_type(true), __indices, __f);
}

} // namespace simd

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___SIMD_PERMUTE_H
