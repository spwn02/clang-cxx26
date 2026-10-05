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

#include <__assert>
#include <__config>
#include <__cstddef/size_t.h>
#include <__mdspan/extents.h>
#include <__mdspan/layout_left.h>
#include <__mdspan/layout_left_padded.h>
#include <__mdspan/layout_padded_helpers.h>
#include <__mdspan/layout_right.h>
#include <__mdspan/layout_right_padded.h>
#include <__mdspan/layout_stride.h>
#include <__mdspan/mdspan.h>
#include <__mdspan/submdspan.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/integer_sequence.h>
#include <array>
#include <tuple>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

// [mdspan.sub.map.result]
template <class _LayoutMapping>
struct submdspan_mapping_result {
  [[no_unique_address]] _LayoutMapping mapping = _LayoutMapping();
  size_t offset{};
};

namespace __mdspan_detail {
template <class _Tp>
inline constexpr bool __is_submdspan_mapping_result = false;
template <class _LayoutMapping>
inline constexpr bool __is_submdspan_mapping_result<submdspan_mapping_result<_LayoutMapping>> = true;

// [mdspan.sub.map.sliceable]: sliceable-mapping. submdspan_mapping is found by argument-dependent lookup only.
template <class _LayoutMapping, size_t... _Is>
consteval bool __is_sliceable_mapping(index_sequence<_Is...>) {
  return requires(const _LayoutMapping& __lm) {
    submdspan_mapping(__lm, ((void)_Is, full_extent)...);
    requires __is_submdspan_mapping_result<remove_cvref_t<decltype(submdspan_mapping(__lm, ((void)_Is, full_extent)...))>>;
  };
}

template <class _LayoutMapping>
concept __sliceable_mapping =
    __is_sliceable_mapping<_LayoutMapping>(make_index_sequence<_LayoutMapping::extents_type::rank()>{});

// Classification of a canonical slice type.
enum class __slice_kind { __collapsing, __full_slice, __unit_stride, __strided };

template <class _Slice>
consteval __slice_kind __slice_kind_of() {
  using _Type = remove_cvref_t<_Slice>;
  if constexpr (is_same_v<_Type, full_extent_t>) {
    return __slice_kind::__full_slice;
  } else if constexpr (__is_extent_slice<_Type>) {
    // [mdspan.sub.overview]: unit-stride slice type.
    if constexpr (__is_constant_wrapper<typename _Type::stride_type>) {
      if (_Type::stride_type::value == 1)
        return __slice_kind::__unit_stride;
    }
    return __slice_kind::__strided;
  } else {
    return __slice_kind::__collapsing;
  }
}

consteval bool __is_unit_stride_kind(__slice_kind __k) {
  return __k == __slice_kind::__full_slice || __k == __slice_kind::__unit_stride;
}

inline constexpr size_t __no_position = static_cast<size_t>(-1);

// Product of the static extents in [__lo, __hi); dynamic_extent if any is dynamic. (An empty product is one.)
template <size_t _Rank>
consteval size_t __static_product(const array<size_t, _Rank>& __statics, size_t __lo, size_t __hi, size_t __seed = 1) {
  size_t __result = __seed;
  for (size_t __k = __lo; __k < __hi; ++__k) {
    if (__result == dynamic_extent || __statics[__k] == dynamic_extent)
      return dynamic_extent;
    __result *= __statics[__k];
  }
  return __result;
}

// [mdspan.sub.map.left]: all of the first SubRank - 1 slices are full_extent_t and slice SubRank - 1 is a
// unit-stride slice type (SubRank >= 1).
template <size_t _Rank>
consteval bool __is_left_contiguous(const array<__slice_kind, _Rank>& __kinds, size_t __sub_rank) {
  for (size_t __k = 0; __k + 1 < __sub_rank; ++__k)
    if (__kinds[__k] != __slice_kind::__full_slice)
      return false;
  return __is_unit_stride_kind(__kinds[__sub_rank - 1]);
}

// [mdspan.sub.map.left]: returns u + 1, where u + 1 is the smallest position p > 0 of a unit-stride slice type, if
// the conditions for a layout_left_padded result hold; __no_position otherwise (SubRank >= 1).
template <size_t _Rank>
consteval size_t __left_padded_position(const array<__slice_kind, _Rank>& __kinds, size_t __sub_rank) {
  if (!__is_unit_stride_kind(__kinds[0]))
    return __no_position;
  size_t __p = 1;
  while (__p < _Rank && !__is_unit_stride_kind(__kinds[__p]))
    ++__p;
  if (__p >= _Rank)
    return __no_position;
  // u == __p - 1; the last slice is at u + SubRank - 1 == __p + SubRank - 2.
  const size_t __last = __p + __sub_rank - 2;
  if (__sub_rank < 2 || __last >= _Rank)
    return __no_position;
  for (size_t __k = __p; __k < __last; ++__k)
    if (__kinds[__k] != __slice_kind::__full_slice)
      return __no_position;
  return __is_unit_stride_kind(__kinds[__last]) ? __p : __no_position;
}

// [mdspan.sub.map.right]: all of the last SubRank - 1 slices are full_extent_t and slice Rank - SubRank is a
// unit-stride slice type (SubRank >= 1).
template <size_t _Rank>
consteval bool __is_right_contiguous(const array<__slice_kind, _Rank>& __kinds, size_t __sub_rank) {
  for (size_t __k = _Rank - __sub_rank + 1; __k < _Rank; ++__k)
    if (__kinds[__k] != __slice_kind::__full_slice)
      return false;
  return __is_unit_stride_kind(__kinds[_Rank - __sub_rank]);
}

// [mdspan.sub.map.right]: returns p == rank_ - u - 2, the largest position p < rank_ - 1 of a unit-stride slice type,
// if the conditions for a layout_right_padded result hold; __no_position otherwise (SubRank >= 1).
template <size_t _Rank>
consteval size_t __right_padded_position(const array<__slice_kind, _Rank>& __kinds, size_t __sub_rank) {
  if (_Rank < 2 || !__is_unit_stride_kind(__kinds[_Rank - 1]))
    return __no_position;
  size_t __p = _Rank - 2;
  while (!__is_unit_stride_kind(__kinds[__p])) {
    if (__p == 0)
      return __no_position;
    --__p;
  }
  // The first slice of the run is at rank_ - SubRank - u == p + 2 - SubRank.
  if (__sub_rank < 2 || __p + 2 < __sub_rank)
    return __no_position;
  const size_t __first = __p + 2 - __sub_rank;
  for (size_t __k = __first + 1; __k <= __p; ++__k)
    if (__kinds[__k] != __slice_kind::__full_slice)
      return __no_position;
  return __is_unit_stride_kind(__kinds[__first]) ? __p : __no_position;
}

template <class _SubExtents>
struct __submdspan_common_result {
  _SubExtents __extents;
  size_t __offset;
  array<typename _SubExtents::index_type, _SubExtents::rank()> __strides;
};

template <class _IndexType, class _Slice>
_LIBCPP_HIDE_FROM_ABI constexpr _IndexType __lower_bound(const _Slice& __s, _IndexType) noexcept {
  if constexpr (is_same_v<_Slice, full_extent_t>)
    return _IndexType(0);
  else if constexpr (__is_extent_slice<_Slice>)
    return static_cast<_IndexType>(__s.offset);
  else
    return static_cast<_IndexType>(__s);
}

// [mdspan.sub.map.common]: Mandates, Expects, sub_ext, sub_strides and offset.
template <class _Mapping, class... _Slices>
_LIBCPP_HIDE_FROM_ABI constexpr auto __submdspan_prepare(const _Mapping& __mapping, const _Slices&... __slices) {
  using _Extents = typename _Mapping::extents_type;
  using _IndexType = typename _Extents::index_type;
  static_assert(__are_valid_slice_types<_Extents, _Slices...>,
                "submdspan_mapping: every slice specifier must be a valid submdspan slice type for the corresponding "
                "extent");
  _LIBCPP_ASSERT_UNCATEGORIZED(__are_valid_slices(__mapping.extents(), __slices...),
                               "submdspan_mapping: every slice must be a valid submdspan slice for the corresponding extent");
  using _Info = __subextents_info<_IndexType, _Extents, _Slices...>;

  auto __sub_ext = __subextents_of_canonical(__mapping.extents(), __slices...);
  array<_IndexType, _Info::__sub_rank> __strides{};
  [&]<size_t... _Ks>(index_sequence<_Ks...>) {
    auto __set = [&]<size_t _Kdx>(const auto& __slice) {
      using _Slice = remove_cvref_t<decltype(__slice)>;
      if constexpr (is_same_v<_Slice, full_extent_t>) {
        __strides[_Info::__map[_Kdx]] = __mapping.stride(_Kdx);
      } else if constexpr (__is_extent_slice<_Slice>) {
        const _IndexType __e = static_cast<_IndexType>(__slice.extent);
        __strides[_Info::__map[_Kdx]] =
            __e > 1 ? static_cast<_IndexType>(__mapping.stride(_Kdx) * static_cast<_IndexType>(__slice.stride))
                    : __mapping.stride(_Kdx);
      }
    };
    (__set.template operator()<_Ks>(__slices), ...);
  }(make_index_sequence<sizeof...(_Slices)>{});

  const size_t __offset = [&]<size_t... _Ks>(index_sequence<_Ks...>) -> size_t {
    const array<_IndexType, sizeof...(_Slices)> __ls = {__lower_bound(__slices, _IndexType())...};
    bool __at_end = false;
    ((__at_end = __at_end || __ls[_Ks] == __mapping.extents().extent(_Ks)), ...);
    if (__at_end)
      return static_cast<size_t>(__mapping.required_span_size());
    return static_cast<size_t>(__mapping(__ls[_Ks]...));
  }(make_index_sequence<sizeof...(_Slices)>{});

  return __submdspan_common_result<decltype(__sub_ext)>{__sub_ext, __offset, __strides};
}

template <class _Extents, class _Mapping>
consteval array<size_t, _Extents::rank()> __static_extents_of() {
  return []<size_t... _Ks>(index_sequence<_Ks...>) {
    return array<size_t, _Extents::rank()>{_Extents::static_extent(_Ks)...};
  }(make_index_sequence<_Extents::rank()>{});
}

template <class _SubExtents, class _Mapping>
_LIBCPP_HIDE_FROM_ABI constexpr auto __submdspan_as_stride(const __submdspan_common_result<_SubExtents>& __common) {
  using _Result = layout_stride::mapping<_SubExtents>;
  return submdspan_mapping_result<_Result>{_Result(__common.__extents, __common.__strides), __common.__offset};
}
} // namespace __mdspan_detail

// [mdspan.sub.map.left]
template <class _Extents>
template <class... _SliceSpecifiers>
_LIBCPP_HIDE_FROM_ABI constexpr auto layout_left::mapping<_Extents>::__submdspan_mapping_impl(_SliceSpecifiers... __slices) const {
  if constexpr (_Extents::rank() == 0) {
    return submdspan_mapping_result<mapping>{*this, 0};
  } else {
    auto __common = __mdspan_detail::__submdspan_prepare(*this, __slices...);
    using _SubExtents                = decltype(__common.__extents);
    constexpr size_t __sub_rank      = _SubExtents::rank();
    constexpr auto __kinds           = array<__mdspan_detail::__slice_kind, _Extents::rank()>{
        __mdspan_detail::__slice_kind_of<_SliceSpecifiers>()...};
    if constexpr (__sub_rank == 0 || __mdspan_detail::__is_left_contiguous(__kinds, __sub_rank)) {
      using _Result = layout_left::mapping<_SubExtents>;
      return submdspan_mapping_result<_Result>{_Result(__common.__extents), __common.__offset};
    } else if constexpr (__mdspan_detail::__left_padded_position(__kinds, __sub_rank) != __mdspan_detail::__no_position) {
      constexpr size_t __p        = __mdspan_detail::__left_padded_position(__kinds, __sub_rank);
      constexpr size_t __s_static = __mdspan_detail::__static_product(
          __mdspan_detail::__static_extents_of<_Extents, mapping>(), 0, __p);
      using _Result = typename layout_left_padded<__s_static>::template mapping<_SubExtents>;
      return submdspan_mapping_result<_Result>{_Result(__common.__extents, this->stride(__p)), __common.__offset};
    } else {
      return __mdspan_detail::__submdspan_as_stride<_SubExtents, mapping>(__common);
    }
  }
}

// [mdspan.sub.map.right]
template <class _Extents>
template <class... _SliceSpecifiers>
_LIBCPP_HIDE_FROM_ABI constexpr auto layout_right::mapping<_Extents>::__submdspan_mapping_impl(_SliceSpecifiers... __slices) const {
  if constexpr (_Extents::rank() == 0) {
    return submdspan_mapping_result<mapping>{*this, 0};
  } else {
    auto __common = __mdspan_detail::__submdspan_prepare(*this, __slices...);
    using _SubExtents                = decltype(__common.__extents);
    constexpr size_t __sub_rank      = _SubExtents::rank();
    constexpr auto __kinds           = array<__mdspan_detail::__slice_kind, _Extents::rank()>{
        __mdspan_detail::__slice_kind_of<_SliceSpecifiers>()...};
    if constexpr (__sub_rank == 0 || __mdspan_detail::__is_right_contiguous(__kinds, __sub_rank)) {
      using _Result = layout_right::mapping<_SubExtents>;
      return submdspan_mapping_result<_Result>{_Result(__common.__extents), __common.__offset};
    } else if constexpr (__mdspan_detail::__right_padded_position(__kinds, __sub_rank) != __mdspan_detail::__no_position) {
      constexpr size_t __p        = __mdspan_detail::__right_padded_position(__kinds, __sub_rank);
      constexpr size_t __s_static = __mdspan_detail::__static_product(
          __mdspan_detail::__static_extents_of<_Extents, mapping>(), __p + 1, _Extents::rank());
      using _Result = typename layout_right_padded<__s_static>::template mapping<_SubExtents>;
      return submdspan_mapping_result<_Result>{_Result(__common.__extents, this->stride(__p)), __common.__offset};
    } else {
      return __mdspan_detail::__submdspan_as_stride<_SubExtents, mapping>(__common);
    }
  }
}

// [mdspan.sub.map.stride]
template <class _Extents>
template <class... _SliceSpecifiers>
_LIBCPP_HIDE_FROM_ABI constexpr auto layout_stride::mapping<_Extents>::__submdspan_mapping_impl(_SliceSpecifiers... __slices) const {
  if constexpr (_Extents::rank() == 0) {
    return submdspan_mapping_result<mapping>{*this, 0};
  } else {
    auto __common = __mdspan_detail::__submdspan_prepare(*this, __slices...);
    return __mdspan_detail::__submdspan_as_stride<decltype(__common.__extents), mapping>(__common);
  }
}

// [mdspan.sub.map.leftpad]
template <size_t _PaddingValue>
template <class _Extents>
template <class... _SliceSpecifiers>
_LIBCPP_HIDE_FROM_ABI constexpr auto
layout_left_padded<_PaddingValue>::mapping<_Extents>::__submdspan_mapping_impl(_SliceSpecifiers... __slices) const {
  if constexpr (_Extents::rank() == 0) {
    return submdspan_mapping_result<mapping>{*this, 0};
  } else {
    auto __common = __mdspan_detail::__submdspan_prepare(*this, __slices...);
    using _SubExtents                = decltype(__common.__extents);
    constexpr size_t __sub_rank      = _SubExtents::rank();
    constexpr auto __kinds           = array<__mdspan_detail::__slice_kind, _Extents::rank()>{
        __mdspan_detail::__slice_kind_of<_SliceSpecifiers>()...};
    // [mdspan.sub.map.leftpad]/2 returns layout_left for every slice of a rank-one mapping. That is only correct when
    // the slice has unit stride (a strided slice would lose its stride), so a strided slice is treated like any other
    // result that is not contiguous.
    constexpr bool __is_contiguous =
        __sub_rank == 0 || (_Extents::rank() == 1 ? __mdspan_detail::__is_unit_stride_kind(__kinds[0])
                                                  : (__sub_rank == 1 && __mdspan_detail::__is_unit_stride_kind(__kinds[0])));
    if constexpr (__is_contiguous) {
      using _Result = layout_left::mapping<_SubExtents>;
      return submdspan_mapping_result<_Result>{_Result(__common.__extents), __common.__offset};
    } else if constexpr (__mdspan_detail::__left_padded_position(__kinds, __sub_rank) != __mdspan_detail::__no_position) {
      constexpr size_t __p = __mdspan_detail::__left_padded_position(__kinds, __sub_rank);
      constexpr size_t __s_static =
          __mdspan_detail::__static_product(__mdspan_detail::__static_extents_of<_Extents, mapping>(),
                                            1,
                                            __p,
                                            __mdspan_detail::__padded_static_stride<mapping, true>);
      using _Result = typename layout_left_padded<__s_static>::template mapping<_SubExtents>;
      return submdspan_mapping_result<_Result>{_Result(__common.__extents, this->stride(__p)), __common.__offset};
    } else {
      return __mdspan_detail::__submdspan_as_stride<_SubExtents, mapping>(__common);
    }
  }
}

// [mdspan.sub.map.rightpad]
template <size_t _PaddingValue>
template <class _Extents>
template <class... _SliceSpecifiers>
_LIBCPP_HIDE_FROM_ABI constexpr auto
layout_right_padded<_PaddingValue>::mapping<_Extents>::__submdspan_mapping_impl(_SliceSpecifiers... __slices) const {
  if constexpr (_Extents::rank() == 0) {
    return submdspan_mapping_result<mapping>{*this, 0};
  } else {
    auto __common = __mdspan_detail::__submdspan_prepare(*this, __slices...);
    using _SubExtents                = decltype(__common.__extents);
    constexpr size_t __sub_rank      = _SubExtents::rank();
    constexpr size_t __rank          = _Extents::rank();
    constexpr auto __kinds           = array<__mdspan_detail::__slice_kind, __rank>{
        __mdspan_detail::__slice_kind_of<_SliceSpecifiers>()...};
    // See layout_left_padded: a strided slice of a rank-one mapping cannot return layout_right.
    constexpr bool __is_contiguous =
        __sub_rank == 0 || (__rank == 1 ? __mdspan_detail::__is_unit_stride_kind(__kinds[0])
                                        : (__sub_rank == 1 && __mdspan_detail::__is_unit_stride_kind(__kinds[__rank - 1])));
    if constexpr (__is_contiguous) {
      using _Result = layout_right::mapping<_SubExtents>;
      return submdspan_mapping_result<_Result>{_Result(__common.__extents), __common.__offset};
    } else if constexpr (__mdspan_detail::__right_padded_position(__kinds, __sub_rank) != __mdspan_detail::__no_position) {
      constexpr size_t __p = __mdspan_detail::__right_padded_position(__kinds, __sub_rank);
      constexpr size_t __s_static =
          __mdspan_detail::__static_product(__mdspan_detail::__static_extents_of<_Extents, mapping>(),
                                            __p + 1,
                                            __rank - 1,
                                            __mdspan_detail::__padded_static_stride<mapping, false>);
      using _Result = typename layout_right_padded<__s_static>::template mapping<_SubExtents>;
      return submdspan_mapping_result<_Result>{_Result(__common.__extents, this->stride(__p)), __common.__offset};
    } else {
      return __mdspan_detail::__submdspan_as_stride<_SubExtents, mapping>(__common);
    }
  }
}

// [mdspan.sub.sub]
template <class _ElementType, class _Extents, class _LayoutPolicy, class _AccessorPolicy, class... _SliceSpecifiers>
  requires(sizeof...(_SliceSpecifiers) == _Extents::rank() &&
           __mdspan_detail::__sliceable_mapping<typename _LayoutPolicy::template mapping<_Extents>>)
_LIBCPP_HIDE_FROM_ABI constexpr auto
submdspan(const mdspan<_ElementType, _Extents, _LayoutPolicy, _AccessorPolicy>& __src, _SliceSpecifiers... __raw_slices) {
  auto __slices = std::canonical_slices(__src.extents(), static_cast<_SliceSpecifiers&&>(__raw_slices)...);
  auto __sub_map_result = std::apply(
      [&](const auto&... __canonical) { return submdspan_mapping(__src.mapping(), __canonical...); }, __slices);
  return mdspan(__src.accessor().offset(__src.data_handle(), __sub_map_result.offset),
                __sub_map_result.mapping,
                typename _AccessorPolicy::offset_policy(__src.accessor()));
}

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___MDSPAN_SUBMDSPAN_MAPPING_H
