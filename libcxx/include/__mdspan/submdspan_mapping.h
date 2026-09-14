// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___MDSPAN_SUBMDSPAN_MAPPING_H
#define _LIBCPP___MDSPAN_SUBMDSPAN_MAPPING_H

#include <__config>
#include <__mdspan/layout_stride.h>
#include <__mdspan/submdspan.h>
#include <__type_traits/remove_cvref.h>
#include <array>
#include <tuple>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

template <class _LayoutMapping>
struct submdspan_mapping_result {
  _LayoutMapping mapping;
  size_t offset;
};

namespace __mdspan_detail {
template <size_t _OutputRank, class _Mapping, class _Tuple, size_t... _Pos>
_LIBCPP_HIDE_FROM_ABI constexpr auto
__submdspan_strides(const _Mapping& __mapping, const _Tuple& __slices, index_sequence<_Pos...>) {
  array<typename _Mapping::index_type, _OutputRank> __strides{};
  size_t __out_pos = 0;
  (([&] {
     using _Slice = remove_cvref_t<decltype(get<_Pos>(__slices))>;
     if constexpr (__extent_slice<_Slice> || is_same_v<_Slice, full_extent_t>) {
       if constexpr (is_same_v<_Slice, full_extent_t>)
         __strides[__out_pos++] = __mapping.stride(_Pos);
       else
         __strides[__out_pos++] = __mapping.stride(_Pos) *
                                  static_cast<typename _Mapping::index_type>(get<_Pos>(__slices).stride);
     }
   }()), ...);
  return __strides;
}

template <class _Mapping, class _Tuple, size_t... _Pos>
_LIBCPP_HIDE_FROM_ABI constexpr auto
__submdspan_offset_indices(const _Tuple& __slices, index_sequence<_Pos...>) {
  return array<typename _Mapping::index_type, sizeof...(_Pos)>{
      [&] {
        using _Slice = remove_cvref_t<decltype(get<_Pos>(__slices))>;
        if constexpr (__extent_slice<_Slice>)
          return static_cast<typename _Mapping::index_type>(get<_Pos>(__slices).offset);
        else if constexpr (is_same_v<_Slice, full_extent_t>)
          return typename _Mapping::index_type(0);
        else
          return static_cast<typename _Mapping::index_type>(get<_Pos>(__slices));
      }()...};
}

template <class _Mapping, class _Tuple, size_t... _Pos>
_LIBCPP_HIDE_FROM_ABI constexpr typename _Mapping::index_type
__submdspan_offset_impl(const _Mapping& __mapping, const _Tuple& __slices, index_sequence<_Pos...>) {
  auto __indices = __submdspan_offset_indices<_Mapping>(__slices, index_sequence<_Pos...>{});
  return [&]<size_t... _I>(index_sequence<_I...>) { return __mapping(__indices[_I]...); }(
      index_sequence<_Pos...>{});
}
} // namespace __mdspan_detail

template <class _LayoutMapping, class... _SliceSpecifiers>
  requires(sizeof...(_SliceSpecifiers) == _LayoutMapping::extents_type::rank())
_LIBCPP_HIDE_FROM_ABI constexpr auto submdspan_mapping(const _LayoutMapping& __mapping,
                                                      _SliceSpecifiers... __slices) {
  auto __canonical = canonical_slices(__mapping.extents(), std::move(__slices)...);
  using _SubExtents = decltype(subextents(__mapping.extents(), std::move(__slices)...));
  using _IndexType  = typename _LayoutMapping::index_type;
  constexpr size_t _Rank = _LayoutMapping::extents_type::rank();
  auto __strides = __mdspan_detail::__submdspan_strides<_SubExtents::rank()>(
      __mapping, __canonical, make_index_sequence<_Rank>{});
  auto __offset = __mdspan_detail::__submdspan_offset_impl(__mapping, __canonical, make_index_sequence<_Rank>{});
  return submdspan_mapping_result<typename layout_stride::template mapping<_SubExtents>>{
      typename layout_stride::template mapping<_SubExtents>(subextents(__mapping.extents(), std::move(__slices)...),
                                                            span(__strides)),
      static_cast<size_t>(__offset)};
}

template <class _Extents, class... _SliceSpecifiers>
  requires(sizeof...(_SliceSpecifiers) == _Extents::rank() &&
           (is_same_v<remove_cvref_t<_SliceSpecifiers>, full_extent_t> && ...))
_LIBCPP_HIDE_FROM_ABI constexpr auto submdspan_mapping(
    const layout_right::mapping<_Extents>& __mapping, _SliceSpecifiers... __slices) {
  using _SubExtents = decltype(subextents(__mapping.extents(), __slices...));
  return submdspan_mapping_result<typename layout_right::template mapping<_SubExtents>>{
      typename layout_right::template mapping<_SubExtents>(subextents(__mapping.extents(), __slices...)), 0};
}

template <class _Extents, class... _SliceSpecifiers>
  requires(sizeof...(_SliceSpecifiers) == _Extents::rank() &&
           (is_same_v<remove_cvref_t<_SliceSpecifiers>, full_extent_t> && ...))
_LIBCPP_HIDE_FROM_ABI constexpr auto submdspan_mapping(
    const layout_left::mapping<_Extents>& __mapping, _SliceSpecifiers... __slices) {
  using _SubExtents = decltype(subextents(__mapping.extents(), __slices...));
  return submdspan_mapping_result<typename layout_left::template mapping<_SubExtents>>{
      typename layout_left::template mapping<_SubExtents>(subextents(__mapping.extents(), __slices...)), 0};
}

template <class _ElementType, class _Extents, class _LayoutPolicy, class _AccessorPolicy, class... _SliceSpecifiers>
  requires(sizeof...(_SliceSpecifiers) == _Extents::rank())
_LIBCPP_HIDE_FROM_ABI constexpr auto submdspan(
    const mdspan<_ElementType, _Extents, _LayoutPolicy, _AccessorPolicy>& __src,
    _SliceSpecifiers... __slices) {
  auto __result = submdspan_mapping(__src.mapping(), std::move(__slices)...);
  using _Result = decltype(__result.mapping);
  using _ResultExtents = typename _Result::extents_type;
  using _ResultLayout = typename _Result::layout_type;
  using _ResultAccessor = typename _AccessorPolicy::offset_policy;
  using _ResultMdspan = mdspan<_ElementType, _ResultExtents, _ResultLayout, _ResultAccessor>;
  return _ResultMdspan(__src.accessor().offset(__src.data_handle(), __result.offset),
                       __result.mapping, _ResultAccessor(__src.accessor()));
}

#endif

_LIBCPP_END_NAMESPACE_STD

#endif
