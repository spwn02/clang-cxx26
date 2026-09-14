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
template <class _Tp>
inline constexpr bool __is_unit_stride_slice = false;
template <class _Tp>
inline constexpr bool __is_constant_unit_stride = false;
template <auto _Value, class _Type>
inline constexpr bool __is_constant_unit_stride<constant_wrapper<_Value, _Type>> = _Value == 1;
template <class _OffsetType, class _ExtentType, class _StrideType>
inline constexpr bool __is_unit_stride_slice<extent_slice<_OffsetType, _ExtentType, _StrideType>> =
    __is_constant_unit_stride<_StrideType>;
template <>
inline constexpr bool __is_unit_stride_slice<full_extent_t> = true;

template <class _Tp>
inline constexpr bool __is_collapsing_slice =
    !is_same_v<_Tp, full_extent_t> && !__extent_slice<_Tp>;

template <class _Tuple, size_t... _Pos>
consteval bool __layout_left_submdspan(index_sequence<_Pos...>) {
  constexpr array<bool, sizeof...(_Pos)> __survives = {
      !__is_collapsing_slice<remove_cvref_t<decltype(get<_Pos>(declval<_Tuple>()))>>...};
  constexpr array<bool, sizeof...(_Pos)> __unit = {
      __is_unit_stride_slice<remove_cvref_t<decltype(get<_Pos>(declval<_Tuple>()))>>...};
  constexpr array<bool, sizeof...(_Pos)> __full = {
      is_same_v<remove_cvref_t<decltype(get<_Pos>(declval<_Tuple>()))>, full_extent_t>...};
  size_t __last = 0;
  bool __found = false;
  for (size_t __i = 0; __i < __survives.size(); ++__i)
    if (__survives[__i]) {
      __last  = __i;
      __found = true;
    }
  if (!__found || !__unit[__last])
    return false;
  for (size_t __i = 0; __i < __last; ++__i)
    if (__survives[__i] && !__full[__i])
      return false;
  return true;
}

template <class _Tuple, size_t... _Pos>
consteval bool __layout_right_submdspan(index_sequence<_Pos...>) {
  constexpr array<bool, sizeof...(_Pos)> __survives = {
      !__is_collapsing_slice<remove_cvref_t<decltype(get<_Pos>(declval<_Tuple>()))>>...};
  constexpr array<bool, sizeof...(_Pos)> __unit = {
      __is_unit_stride_slice<remove_cvref_t<decltype(get<_Pos>(declval<_Tuple>()))>>...};
  constexpr array<bool, sizeof...(_Pos)> __full = {
      is_same_v<remove_cvref_t<decltype(get<_Pos>(declval<_Tuple>()))>, full_extent_t>...};
  size_t __first = 0;
  bool __found = false;
  for (size_t __i = 0; __i < __survives.size(); ++__i)
    if (__survives[__i]) {
      __first = __i;
      __found = true;
      break;
    }
  if (!__found || !__unit[__first])
    return false;
  for (size_t __i = __first + 1; __i < __survives.size(); ++__i)
    if (__survives[__i] && !__full[__i])
      return false;
  return true;
}

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
         __strides[__out_pos++] = get<_Pos>(__slices).extent > 1
                                      ? __mapping.stride(_Pos) *
                                            static_cast<typename _Mapping::index_type>(get<_Pos>(__slices).stride)
                                      : __mapping.stride(_Pos);
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
  requires(sizeof...(_SliceSpecifiers) == _Extents::rank())
_LIBCPP_HIDE_FROM_ABI constexpr auto submdspan_mapping(
    const layout_right::mapping<_Extents>& __mapping, _SliceSpecifiers... __slices) {
  auto __canonical = canonical_slices(__mapping.extents(), std::move(__slices)...);
  using _SubExtents = decltype(subextents(__mapping.extents(), __slices...));
  auto __offset = __mdspan_detail::__submdspan_offset_impl(__mapping, __canonical, make_index_sequence<_Extents::rank()>{});
  if constexpr (_SubExtents::rank() == 0 ||
                __mdspan_detail::__layout_right_submdspan<decltype(__canonical)>(make_index_sequence<_Extents::rank()>{}))
    return submdspan_mapping_result<typename layout_right::template mapping<_SubExtents>>{
        typename layout_right::template mapping<_SubExtents>(subextents(__mapping.extents(), __slices...)),
        static_cast<size_t>(__offset)};
  else {
    auto __strides = __mdspan_detail::__submdspan_strides<_SubExtents::rank()>(
        __mapping, __canonical, make_index_sequence<_Extents::rank()>{});
    return submdspan_mapping_result<typename layout_stride::template mapping<_SubExtents>>{
        typename layout_stride::template mapping<_SubExtents>(subextents(__mapping.extents(), __slices...),
                                                               span(__strides)),
        static_cast<size_t>(__offset)};
  }
}

template <size_t _Padding, class _Extents, class... _SliceSpecifiers>
  requires(sizeof...(_SliceSpecifiers) == _Extents::rank())
_LIBCPP_HIDE_FROM_ABI constexpr auto submdspan_mapping(
    const typename layout_left_padded<_Padding>::template mapping<_Extents>& __mapping,
    _SliceSpecifiers... __slices) {
  auto __canonical = canonical_slices(__mapping.extents(), std::move(__slices)...);
  using _SubExtents = decltype(subextents(__mapping.extents(), __slices...));
  auto __offset = __mdspan_detail::__submdspan_offset_impl(__mapping, __canonical, make_index_sequence<_Extents::rank()>{});
  if constexpr (_SubExtents::rank() == 0)
    return submdspan_mapping_result<typename layout_left::template mapping<_SubExtents>>{
        typename layout_left::template mapping<_SubExtents>(subextents(__mapping.extents(), __slices...)),
        static_cast<size_t>(__offset)};
  else {
    auto __strides = __mdspan_detail::__submdspan_strides<_SubExtents::rank()>(
        __mapping, __canonical, make_index_sequence<_Extents::rank()>{});
    return submdspan_mapping_result<typename layout_stride::template mapping<_SubExtents>>{
        typename layout_stride::template mapping<_SubExtents>(subextents(__mapping.extents(), __slices...), span(__strides)),
        static_cast<size_t>(__offset)};
  }
}

template <size_t _Padding, class _Extents, class... _SliceSpecifiers>
  requires(sizeof...(_SliceSpecifiers) == _Extents::rank())
_LIBCPP_HIDE_FROM_ABI constexpr auto submdspan_mapping(
    const typename layout_right_padded<_Padding>::template mapping<_Extents>& __mapping,
    _SliceSpecifiers... __slices) {
  auto __canonical = canonical_slices(__mapping.extents(), std::move(__slices)...);
  using _SubExtents = decltype(subextents(__mapping.extents(), __slices...));
  auto __offset = __mdspan_detail::__submdspan_offset_impl(__mapping, __canonical, make_index_sequence<_Extents::rank()>{});
  if constexpr (_SubExtents::rank() == 0)
    return submdspan_mapping_result<typename layout_right::template mapping<_SubExtents>>{
        typename layout_right::template mapping<_SubExtents>(subextents(__mapping.extents(), __slices...)),
        static_cast<size_t>(__offset)};
  else {
    auto __strides = __mdspan_detail::__submdspan_strides<_SubExtents::rank()>(
        __mapping, __canonical, make_index_sequence<_Extents::rank()>{});
    return submdspan_mapping_result<typename layout_stride::template mapping<_SubExtents>>{
        typename layout_stride::template mapping<_SubExtents>(subextents(__mapping.extents(), __slices...), span(__strides)),
        static_cast<size_t>(__offset)};
  }
}

template <class _Extents, class... _SliceSpecifiers>
  requires(sizeof...(_SliceSpecifiers) == _Extents::rank())
_LIBCPP_HIDE_FROM_ABI constexpr auto submdspan_mapping(
    const layout_left::mapping<_Extents>& __mapping, _SliceSpecifiers... __slices) {
  auto __canonical = canonical_slices(__mapping.extents(), std::move(__slices)...);
  using _SubExtents = decltype(subextents(__mapping.extents(), __slices...));
  auto __offset = __mdspan_detail::__submdspan_offset_impl(__mapping, __canonical, make_index_sequence<_Extents::rank()>{});
  if constexpr (_SubExtents::rank() == 0 ||
                __mdspan_detail::__layout_left_submdspan<decltype(__canonical)>(make_index_sequence<_Extents::rank()>{}))
    return submdspan_mapping_result<typename layout_left::template mapping<_SubExtents>>{
        typename layout_left::template mapping<_SubExtents>(subextents(__mapping.extents(), __slices...)),
        static_cast<size_t>(__offset)};
  else {
    auto __strides = __mdspan_detail::__submdspan_strides<_SubExtents::rank()>(
        __mapping, __canonical, make_index_sequence<_Extents::rank()>{});
    return submdspan_mapping_result<typename layout_stride::template mapping<_SubExtents>>{
        typename layout_stride::template mapping<_SubExtents>(subextents(__mapping.extents(), __slices...),
                                                               span(__strides)),
        static_cast<size_t>(__offset)};
  }
}

template <class _ElementType, class _Extents, class _LayoutPolicy, class _AccessorPolicy, class... _SliceSpecifiers>
  requires(sizeof...(_SliceSpecifiers) == _Extents::rank())
_LIBCPP_HIDE_FROM_ABI constexpr auto submdspan(
    const mdspan<_ElementType, _Extents, _LayoutPolicy, _AccessorPolicy>& __src,
    _SliceSpecifiers... __slices) {
  auto __canonical = canonical_slices(__src.extents(), std::move(__slices)...);
  auto __result = [&]<size_t... _I>(index_sequence<_I...>) {
    return submdspan_mapping(__src.mapping(), get<_I>(__canonical)...);
  }(make_index_sequence<sizeof...(_SliceSpecifiers)>{});
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
