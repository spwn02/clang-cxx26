// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___MDSPAN_COPY_FILL_H
#define _LIBCPP___MDSPAN_COPY_FILL_H

#include <__assert>
#include <__config>
#include <__cstddef/size_t.h>
#include <__fwd/mdspan.h>
#include <__mdspan/mdspan.h>
#include <__type_traits/is_assignable.h>
#include <__type_traits/is_constructible.h>
#include <__type_traits/is_execution_policy.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/forward.h>
#include <__utility/integer_sequence.h>
#include <array>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

namespace __mdspan_detail {
template <class _Tp>
inline constexpr bool __is_mdspan_specialization = false;
template <class _ElementType, class _Extents, class _LayoutPolicy, class _AccessorPolicy>
inline constexpr bool __is_mdspan_specialization<mdspan<_ElementType, _Extents, _LayoutPolicy, _AccessorPolicy>> = true;

// Calls __f(__i...) for every multidimensional index __i in __extents, in row-major order.
template <class _Extents, class _Func>
_LIBCPP_HIDE_FROM_ABI constexpr void __for_each_multidimensional_index(const _Extents& __extents, _Func&& __f) {
  constexpr size_t __rank = _Extents::rank();
  if constexpr (__rank == 0) {
    __f();
  } else {
    for (size_t __r = 0; __r < __rank; ++__r)
      if (__extents.extent(__r) == 0)
        return;
    array<typename _Extents::index_type, __rank> __index{};
    while (true) {
      [&]<size_t... _Is>(index_sequence<_Is...>) { __f(__index[_Is]...); }(make_index_sequence<__rank>{});
      size_t __r = __rank;
      while (__r != 0) {
        --__r;
        if (++__index[__r] < __extents.extent(__r))
          break;
        __index[__r] = 0;
        if (__r == 0)
          return;
      }
    }
  }
}

template <class _Src, class _Dst>
concept __mdspan_copyable =
    __is_mdspan_specialization<_Src> && __is_mdspan_specialization<_Dst> &&
    is_assignable_v<typename _Dst::reference, typename _Src::reference> &&
    is_constructible_v<typename _Src::extents_type, typename _Dst::extents_type>;
} // namespace __mdspan_detail

// [mdspan.copy]
template <class _Src, class _Dst>
  requires __mdspan_detail::__mdspan_copyable<_Src, _Dst>
_LIBCPP_HIDE_FROM_ABI constexpr void copy(const _Src& __src, const _Dst& __dst) {
  _LIBCPP_ASSERT_UNCATEGORIZED(__dst.is_unique(), "std::copy(mdspan): the destination mapping must be unique");
  _LIBCPP_ASSERT_ARGUMENT_WITHIN_DOMAIN(
      __src.extents() == __dst.extents(), "std::copy(mdspan): the source and destination extents must be equal");
  __mdspan_detail::__for_each_multidimensional_index(
      __src.extents(), [&]<class... _Indices>(_Indices... __indices) { __dst[__indices...] = __src[__indices...]; });
}

// The execution-policy overloads use the permitted serial fallback.
template <class _ExecutionPolicy, class _Src, class _Dst>
  requires(is_execution_policy_v<remove_cvref_t<_ExecutionPolicy>> && __mdspan_detail::__mdspan_copyable<_Src, _Dst>)
_LIBCPP_HIDE_FROM_ABI void copy(_ExecutionPolicy&&, const _Src& __src, const _Dst& __dst) {
  std::copy(__src, __dst);
}

template <class _Dst, class _Tp = typename _Dst::value_type>
  requires(__mdspan_detail::__is_mdspan_specialization<_Dst> && is_assignable_v<typename _Dst::reference, const _Tp&>)
_LIBCPP_HIDE_FROM_ABI constexpr void fill(const _Dst& __dst, const _Tp& __value) {
  __mdspan_detail::__for_each_multidimensional_index(
      __dst.extents(), [&]<class... _Indices>(_Indices... __indices) { __dst[__indices...] = __value; });
}

template <class _ExecutionPolicy, class _Dst, class _Tp = typename _Dst::value_type>
  requires(is_execution_policy_v<remove_cvref_t<_ExecutionPolicy>> &&
           __mdspan_detail::__is_mdspan_specialization<_Dst> && is_assignable_v<typename _Dst::reference, const _Tp&>)
_LIBCPP_HIDE_FROM_ABI void fill(_ExecutionPolicy&&, const _Dst& __dst, const _Tp& __value) {
  std::fill(__dst, __value);
}

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___MDSPAN_COPY_FILL_H
