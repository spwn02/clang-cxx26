// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___SIMD_LOADSTORE_H
#define _LIBCPP___SIMD_LOADSTORE_H

#include <__assert>
#include <__config>
#include <__cstddef/size_t.h>
#include <__iterator/concepts.h>
#include <__iterator/incrementable_traits.h>
#include <__memory/pointer_traits.h>
#include <__ranges/concepts.h>
#include <__ranges/data.h>
#include <__ranges/size.h>
#include <__simd/abi.h>
#include <__simd/basic_vec.h>
#include <__simd/concepts.h>
#include <__simd/flags.h>
#include <__simd/traits.h>
#include <__type_traits/conditional.h>
#include <__type_traits/is_void.h>
#include <__utility/forward.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

namespace simd {

template <class _Rp, class _Vp>
using __load_default_vec _LIBCPP_NODEBUG = basic_vec<ranges::range_value_t<_Rp>, __native_abi<ranges::range_value_t<_Rp>>>;

template <class _Ip>
using __load_default_vec_iter _LIBCPP_NODEBUG = basic_vec<iter_value_t<_Ip>, __native_abi<iter_value_t<_Ip>>>;

// [simd.loadstore]/6-10: partial_load, range form. This is where the actual bounds/mask logic
// lives; every other load/store overload (unchecked_*, and the iterator-pair forms) funnels
// through this pair, per the clause's own "Effects: Equivalent to" wording for the analogous
// functions.
template <class _Result, ranges::contiguous_range _Rp, class... _Flags>
_LIBCPP_HIDE_FROM_ABI constexpr _Result __partial_load_impl(_Rp&& __r, const auto& __mask, flags<_Flags...>) {
  using _Tp          = typename _Result::value_type;
  auto* __p          = ranges::data(__r);
  const auto __count = static_cast<__simd_size_type>(ranges::size(__r));
  return _Result([&](auto __i) { return (__mask[__i] && __i < __count) ? static_cast<_Tp>(__p[__i]) : _Tp(); });
}

template <class _Vp = void, ranges::contiguous_range _Rp, class... _Flags>
  requires ranges::sized_range<_Rp>
_LIBCPP_HIDE_FROM_ABI constexpr auto
partial_load(_Rp&& __r, const typename conditional_t<is_void_v<_Vp>, __load_default_vec<_Rp, void>, _Vp>::mask_type& __mask,
             flags<_Flags...> __f = {}) {
  using _Result = conditional_t<is_void_v<_Vp>, __load_default_vec<_Rp, void>, _Vp>;
  return __partial_load_impl<_Result>(std::forward<_Rp>(__r), __mask, __f);
}

template <class _Vp = void, ranges::contiguous_range _Rp, class... _Flags>
  requires ranges::sized_range<_Rp>
_LIBCPP_HIDE_FROM_ABI constexpr auto partial_load(_Rp&& __r, flags<_Flags...> __f = {}) {
  using _Result = conditional_t<is_void_v<_Vp>, __load_default_vec<_Rp, void>, _Vp>;
  return __partial_load_impl<_Result>(std::forward<_Rp>(__r), typename _Result::mask_type(true), __f);
}

template <class _Result, contiguous_iterator _Ip, class... _Flags>
_LIBCPP_HIDE_FROM_ABI constexpr _Result
__partial_load_impl(_Ip __first, iter_difference_t<_Ip> __n, const auto& __mask, flags<_Flags...>) {
  using _Tp = typename _Result::value_type;
  auto* __p = std::to_address(__first);
  return _Result([&](auto __i) { return (__mask[__i] && __i < __n) ? static_cast<_Tp>(__p[__i]) : _Tp(); });
}

template <class _Vp = void, contiguous_iterator _Ip, class... _Flags>
_LIBCPP_HIDE_FROM_ABI constexpr auto
partial_load(_Ip __first, iter_difference_t<_Ip> __n, const typename conditional_t<is_void_v<_Vp>, __load_default_vec_iter<_Ip>, _Vp>::mask_type& __mask,
             flags<_Flags...> __f = {}) {
  using _Result = conditional_t<is_void_v<_Vp>, __load_default_vec_iter<_Ip>, _Vp>;
  return __partial_load_impl<_Result>(__first, __n, __mask, __f);
}

template <class _Vp = void, contiguous_iterator _Ip, class... _Flags>
_LIBCPP_HIDE_FROM_ABI constexpr auto partial_load(_Ip __first, iter_difference_t<_Ip> __n, flags<_Flags...> __f = {}) {
  using _Result = conditional_t<is_void_v<_Vp>, __load_default_vec_iter<_Ip>, _Vp>;
  return partial_load<_Result>(__first, __n, typename _Result::mask_type(true), __f);
}

template <class _Vp = void, contiguous_iterator _Ip, sized_sentinel_for<_Ip> _Sp, class... _Flags>
_LIBCPP_HIDE_FROM_ABI constexpr auto
partial_load(_Ip __first, _Sp __last, const auto& __mask, flags<_Flags...> __f = {}) {
  return partial_load<_Vp>(__first, static_cast<iter_difference_t<_Ip>>(__last - __first), __mask, __f);
}

template <class _Vp = void, contiguous_iterator _Ip, sized_sentinel_for<_Ip> _Sp, class... _Flags>
_LIBCPP_HIDE_FROM_ABI constexpr auto partial_load(_Ip __first, _Sp __last, flags<_Flags...> __f = {}) {
  using _Result = conditional_t<is_void_v<_Vp>, __load_default_vec_iter<_Ip>, _Vp>;
  return partial_load<_Result>(__first, static_cast<iter_difference_t<_Ip>>(__last - __first),
                               typename _Result::mask_type(true), __f);
}

// [simd.loadstore]/1-5: unchecked_load -- Effects: Equivalent to partial_load<V>(r, mask, f), with
// the caller additionally guaranteeing ranges::size(r) >= V::size() (a Mandates in the clause; not
// separately enforced here since partial_load's own bounds check already makes the call safe
// either way -- see the KNOWN DEVIATION note in permute.h for the general policy on this class of
// compile-time Mandates in this implementation).
template <class _Vp = void, ranges::contiguous_range _Rp, class... _Flags>
  requires ranges::sized_range<_Rp>
_LIBCPP_HIDE_FROM_ABI constexpr auto unchecked_load(_Rp&& __r, flags<_Flags...> __f = {}) {
  return partial_load<_Vp>(std::forward<_Rp>(__r), __f);
}

template <class _Vp = void, ranges::contiguous_range _Rp, class... _Flags>
  requires ranges::sized_range<_Rp>
_LIBCPP_HIDE_FROM_ABI constexpr auto unchecked_load(_Rp&& __r, const auto& __mask, flags<_Flags...> __f = {}) {
  return partial_load<_Vp>(std::forward<_Rp>(__r), __mask, __f);
}

template <class _Vp = void, contiguous_iterator _Ip, class... _Flags>
_LIBCPP_HIDE_FROM_ABI constexpr auto unchecked_load(_Ip __first, iter_difference_t<_Ip> __n, flags<_Flags...> __f = {}) {
  return partial_load<_Vp>(__first, __n, __f);
}

template <class _Vp = void, contiguous_iterator _Ip, class... _Flags>
_LIBCPP_HIDE_FROM_ABI constexpr auto
unchecked_load(_Ip __first, iter_difference_t<_Ip> __n, const auto& __mask, flags<_Flags...> __f = {}) {
  return partial_load<_Vp>(__first, __n, __mask, __f);
}

template <class _Vp = void, contiguous_iterator _Ip, sized_sentinel_for<_Ip> _Sp, class... _Flags>
_LIBCPP_HIDE_FROM_ABI constexpr auto unchecked_load(_Ip __first, _Sp __last, flags<_Flags...> __f = {}) {
  return partial_load<_Vp>(__first, __last, __f);
}

template <class _Vp = void, contiguous_iterator _Ip, sized_sentinel_for<_Ip> _Sp, class... _Flags>
_LIBCPP_HIDE_FROM_ABI constexpr auto
unchecked_load(_Ip __first, _Sp __last, const auto& __mask, flags<_Flags...> __f = {}) {
  return partial_load<_Vp>(__first, __last, __mask, __f);
}

// [simd.loadstore]: partial_store, range form.
template <class _Tp, class _Abi, ranges::contiguous_range _Rp, class... _Flags>
  requires ranges::sized_range<_Rp>
_LIBCPP_HIDE_FROM_ABI constexpr void
partial_store(const basic_vec<_Tp, _Abi>& __v, _Rp&& __r, const auto& __mask, flags<_Flags...> = {}) {
  auto* __p          = ranges::data(__r);
  const auto __count = static_cast<__simd_size_type>(ranges::size(__r));
  for (__simd_size_type __i = 0; __i != basic_vec<_Tp, _Abi>::size(); ++__i)
    if (__mask[__i] && __i < __count)
      __p[__i] = __v[__i];
}

template <class _Tp, class _Abi, ranges::contiguous_range _Rp, class... _Flags>
  requires ranges::sized_range<_Rp>
_LIBCPP_HIDE_FROM_ABI constexpr void partial_store(const basic_vec<_Tp, _Abi>& __v, _Rp&& __r, flags<_Flags...> __f = {}) {
  partial_store(__v, std::forward<_Rp>(__r), typename basic_vec<_Tp, _Abi>::mask_type(true), __f);
}

template <class _Tp, class _Abi, contiguous_iterator _Ip, class... _Flags>
_LIBCPP_HIDE_FROM_ABI constexpr void
partial_store(const basic_vec<_Tp, _Abi>& __v, _Ip __first, iter_difference_t<_Ip> __n, const auto& __mask,
             flags<_Flags...> = {}) {
  auto* __p = std::to_address(__first);
  for (__simd_size_type __i = 0; __i != basic_vec<_Tp, _Abi>::size(); ++__i)
    if (__mask[__i] && __i < __n)
      __p[__i] = __v[__i];
}

template <class _Tp, class _Abi, contiguous_iterator _Ip, class... _Flags>
_LIBCPP_HIDE_FROM_ABI constexpr void
partial_store(const basic_vec<_Tp, _Abi>& __v, _Ip __first, iter_difference_t<_Ip> __n, flags<_Flags...> __f = {}) {
  partial_store(__v, __first, __n, typename basic_vec<_Tp, _Abi>::mask_type(true), __f);
}

template <class _Tp, class _Abi, contiguous_iterator _Ip, sized_sentinel_for<_Ip> _Sp, class... _Flags>
_LIBCPP_HIDE_FROM_ABI constexpr void
partial_store(const basic_vec<_Tp, _Abi>& __v, _Ip __first, _Sp __last, const auto& __mask, flags<_Flags...> __f = {}) {
  partial_store(__v, __first, static_cast<iter_difference_t<_Ip>>(__last - __first), __mask, __f);
}

template <class _Tp, class _Abi, contiguous_iterator _Ip, sized_sentinel_for<_Ip> _Sp, class... _Flags>
_LIBCPP_HIDE_FROM_ABI constexpr void
partial_store(const basic_vec<_Tp, _Abi>& __v, _Ip __first, _Sp __last, flags<_Flags...> __f = {}) {
  partial_store(__v, __first, static_cast<iter_difference_t<_Ip>>(__last - __first),
               typename basic_vec<_Tp, _Abi>::mask_type(true), __f);
}

// [simd.loadstore]: unchecked_store -- delegates to partial_store, mirroring unchecked_load.
template <class _Tp, class _Abi, ranges::contiguous_range _Rp, class... _Flags>
  requires ranges::sized_range<_Rp>
_LIBCPP_HIDE_FROM_ABI constexpr void unchecked_store(const basic_vec<_Tp, _Abi>& __v, _Rp&& __r, flags<_Flags...> __f = {}) {
  partial_store(__v, std::forward<_Rp>(__r), __f);
}

template <class _Tp, class _Abi, ranges::contiguous_range _Rp, class... _Flags>
  requires ranges::sized_range<_Rp>
_LIBCPP_HIDE_FROM_ABI constexpr void
unchecked_store(const basic_vec<_Tp, _Abi>& __v, _Rp&& __r, const auto& __mask, flags<_Flags...> __f = {}) {
  partial_store(__v, std::forward<_Rp>(__r), __mask, __f);
}

template <class _Tp, class _Abi, contiguous_iterator _Ip, class... _Flags>
_LIBCPP_HIDE_FROM_ABI constexpr void
unchecked_store(const basic_vec<_Tp, _Abi>& __v, _Ip __first, iter_difference_t<_Ip> __n, flags<_Flags...> __f = {}) {
  partial_store(__v, __first, __n, __f);
}

template <class _Tp, class _Abi, contiguous_iterator _Ip, class... _Flags>
_LIBCPP_HIDE_FROM_ABI constexpr void unchecked_store(
    const basic_vec<_Tp, _Abi>& __v, _Ip __first, iter_difference_t<_Ip> __n, const auto& __mask, flags<_Flags...> __f = {}) {
  partial_store(__v, __first, __n, __mask, __f);
}

template <class _Tp, class _Abi, contiguous_iterator _Ip, sized_sentinel_for<_Ip> _Sp, class... _Flags>
_LIBCPP_HIDE_FROM_ABI constexpr void
unchecked_store(const basic_vec<_Tp, _Abi>& __v, _Ip __first, _Sp __last, flags<_Flags...> __f = {}) {
  partial_store(__v, __first, __last, __f);
}

template <class _Tp, class _Abi, contiguous_iterator _Ip, sized_sentinel_for<_Ip> _Sp, class... _Flags>
_LIBCPP_HIDE_FROM_ABI constexpr void
unchecked_store(const basic_vec<_Tp, _Abi>& __v, _Ip __first, _Sp __last, const auto& __mask, flags<_Flags...> __f = {}) {
  partial_store(__v, __first, __last, __mask, __f);
}

} // namespace simd

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___SIMD_LOADSTORE_H
