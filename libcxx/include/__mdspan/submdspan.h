// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___MDSPAN_SUBMDSPAN_H
#define _LIBCPP___MDSPAN_SUBMDSPAN_H

#include <__config>
#include <__mdspan/extents.h>
#include <__type_traits/is_same.h>
#include <__type_traits/is_convertible.h>
#include <__type_traits/remove_cvref.h>
#include <__type_traits/void_t.h>
#include <__utility/constant_wrapper.h>
#include <__utility/declval.h>
#include <__utility/forward.h>
#include <array>
#include <tuple>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

struct full_extent_t {
  explicit constexpr full_extent_t() = default;
};
inline constexpr full_extent_t full_extent{};

template <class _OffsetType, class _ExtentType, class _StrideType>
struct extent_slice {
  using offset_type = _OffsetType;
  using extent_type = _ExtentType;
  using stride_type = _StrideType;
  [[no_unique_address]] offset_type offset{};
  [[no_unique_address]] extent_type extent{};
  [[no_unique_address]] stride_type stride{};
};

template <class _FirstType, class _LastType, class _StrideType = constant_wrapper<1zu>>
struct range_slice {
  [[no_unique_address]] _FirstType first{};
  [[no_unique_address]] _LastType last{};
  [[no_unique_address]] _StrideType stride{};
};

namespace __mdspan_detail {
template <class _Tp, class = void>
struct __integral_constant_like : false_type {};
template <class _Tp>
struct __integral_constant_like<_Tp, void_t<decltype(_Tp::value)>>
    : bool_constant<is_integral_v<remove_cvref_t<decltype(_Tp::value)>> &&
                    !is_same_v<bool, remove_cvref_t<decltype(_Tp::value)>> &&
                    is_convertible_v<_Tp, decltype(_Tp::value)>> {};
template <class _Tp>
concept __constant_wrapper = requires { typename constant_wrapper<_Tp::value>; };
template <class _Tp>
struct __is_extent_slice : false_type {};
template <class _OffsetType, class _ExtentType, class _StrideType>
struct __is_extent_slice<extent_slice<_OffsetType, _ExtentType, _StrideType>> : true_type {};
template <class _Tp>
concept __extent_slice = __is_extent_slice<remove_cvref_t<_Tp>>::value;
template <class _Tp>
struct __is_range_slice : false_type {};
template <class _FirstType, class _LastType, class _StrideType>
struct __is_range_slice<range_slice<_FirstType, _LastType, _StrideType>> : true_type {};
template <class _Tp>
concept __range_slice = __is_range_slice<remove_cvref_t<_Tp>>::value;

template <class _IndexType, class _SpanType, class... _StrideTypes>
struct __range_stride;
template <class _IndexType, class _SpanType>
struct __range_stride<_IndexType, _SpanType> { using type = constant_wrapper<_IndexType(1)>; };
template <class _IndexType, class _SpanType, class _StrideType>
struct __range_stride<_IndexType, _SpanType, _StrideType> {
  using type = conditional_t<__constant_wrapper<_SpanType>, constant_wrapper<_IndexType(1)>, _StrideType>;
};
template <class _Tp, class = void>
struct __static_subextent : integral_constant<size_t, dynamic_extent> {};
template <class _Tp>
struct __cw_static_value : integral_constant<size_t, dynamic_extent> {};
template <auto __value_type, class _ValueType>
struct __cw_static_value<constant_wrapper<__value_type, _ValueType>> : integral_constant<size_t, static_cast<size_t>(__value_type)> {};
template <class _Tp>
struct __static_subextent<_Tp, void_t<typename _Tp::extent_type>>
    : __cw_static_value<typename _Tp::extent_type> {};
template <class _IndexType, class _Tp>
_LIBCPP_HIDE_FROM_ABI constexpr _IndexType __subextent_value(const _Tp& __s) {
  if constexpr (is_same_v<_Tp, full_extent_t>)
    return _IndexType{};
  else
    return _IndexType(__s.extent);
}

template <class _IndexType, class __slice_type>
_LIBCPP_HIDE_FROM_ABI constexpr auto __canonical_index(__slice_type __s) {
  if constexpr (__integral_constant_like<__slice_type>::value)
    return cw<_IndexType(__slice_type::value)>;
  else
    return _IndexType(std::forward<decltype(__s)>(__s));
}

template <class _IndexType, class _OffsetType, class _SpanType, class... _StrideTypes>
_LIBCPP_HIDE_FROM_ABI constexpr auto __canonical_slice_range(_OffsetType __offset, _SpanType __span,
                                                              _StrideTypes... __strides) {
  static_assert(sizeof...(_StrideTypes) <= 1);
  using _StrideType = typename __range_stride<_IndexType, _SpanType, _StrideTypes...>::type;
  _IndexType __stride = [&] {
    if constexpr (__constant_wrapper<_StrideType>) return _StrideType::value;
    else if (__span == 0) return _IndexType(1);
    else return _IndexType(__strides...);
  }();
  if constexpr (__constant_wrapper<_SpanType> && __constant_wrapper<_StrideType>) {
    constexpr _IndexType __extent = _SpanType::value == 0 ? 0 : 1 + (_SpanType::value - 1) / _StrideType::value;
    return extent_slice<_OffsetType, constant_wrapper<__extent>, _StrideType>{__offset, {}, {}};
  } else {
    return extent_slice<_OffsetType, _IndexType, _StrideType>{
        __offset, __span == 0 ? _IndexType(0) : _IndexType(1) + (__span - 1) / __stride, {}};
  }
}

template <class _IndexType, class __slice_type>
_LIBCPP_HIDE_FROM_ABI constexpr auto __canonical_slice(__slice_type __s) {
  if constexpr (is_convertible_v<__slice_type, full_extent_t>)
    return static_cast<full_extent_t>(std::forward<decltype(__s)>(__s));
  else if constexpr (is_convertible_v<__slice_type, _IndexType>)
    return __canonical_index<_IndexType>(std::forward<decltype(__s)>(__s));
  else if constexpr (__extent_slice<__slice_type>)
    return extent_slice{__canonical_index<_IndexType>(std::forward<decltype(__s.offset)>(__s.offset)),
                        __canonical_index<_IndexType>(std::forward<decltype(__s.extent)>(__s.extent)),
                        __canonical_index<_IndexType>(std::forward<decltype(__s.stride)>(__s.stride))};
  else if constexpr (__range_slice<__slice_type>) {
    auto __first = __canonical_index<_IndexType>(std::forward<decltype(__s.first)>(__s.first));
    auto __last  = __canonical_index<_IndexType>(std::forward<decltype(__s.last)>(__s.last));
    return __canonical_slice_range<_IndexType>(__first, __canonical_index<_IndexType>(__last - __first),
                                               __canonical_index<_IndexType>(std::forward<decltype(__s.stride)>(__s.stride)));
  } else {
    auto [__first, __last] = std::forward<decltype(__s)>(__s);
    auto __cfirst = __canonical_index<_IndexType>(std::forward<decltype(__first)>(__first));
    auto __clast  = __canonical_index<_IndexType>(std::forward<decltype(__last)>(__last));
    return __canonical_slice_range<_IndexType>(__cfirst, __canonical_index<_IndexType>(__clast - __cfirst));
  }
}
} // namespace __mdspan_detail

template <class _IndexType, size_t... _Extents, class... _SliceSpecifiers>
  requires(sizeof...(_SliceSpecifiers) == sizeof...(_Extents))
_LIBCPP_HIDE_FROM_ABI constexpr auto canonical_slices(const extents<_IndexType, _Extents...>&,
                                                      _SliceSpecifiers... __slices) {
  return std::tuple{__mdspan_detail::__canonical_slice<_IndexType>(std::forward<decltype(__slices)>(__slices))...};
}

namespace __mdspan_detail {
template <size_t _Pos, size_t... _Extents>
inline constexpr size_t __source_static_extent = array{_Extents...}[_Pos];

template <class _IndexType, class _Tuple, class _SourceSequence, size_t _Pos, size_t... _OutExtents>
struct __subextents_type_impl;

template <bool _Done, class _IndexType, class _Tuple, class _SourceSequence, size_t _Pos, size_t... _OutExtents>
struct __subextents_type_step;

template <class _IndexType, class _Tuple, class _SourceSequence, size_t _Pos, size_t... _OutExtents>
struct __subextents_type_step<true, _IndexType, _Tuple, _SourceSequence, _Pos, _OutExtents...> {
  using type = extents<_IndexType, _OutExtents...>;
};

template <class _IndexType, class _Tuple, size_t... _SourceExtents, size_t _Pos, size_t... _OutExtents>
struct __subextents_type_impl<_IndexType,
                              _Tuple,
                              integer_sequence<size_t, _SourceExtents...>,
                              _Pos,
                              _OutExtents...> {
  using type = typename __subextents_type_step<
      (_Pos == tuple_size_v<remove_reference_t<_Tuple>>),
      _IndexType,
      _Tuple,
      integer_sequence<size_t, _SourceExtents...>,
      _Pos,
      _OutExtents...>::type;
};

template <class _IndexType, class _Tuple, size_t... _SourceExtents, size_t _Pos, size_t... _OutExtents>
struct __subextents_type_step<false,
                              _IndexType,
                              _Tuple,
                              integer_sequence<size_t, _SourceExtents...>,
                              _Pos,
                              _OutExtents...> {
  using _Slice = remove_cvref_t<decltype(get<_Pos>(declval<_Tuple>()))>;
  static constexpr bool __collapsing = !is_same_v<_Slice, full_extent_t> && !__extent_slice<_Slice>;
  using type = conditional_t<__collapsing,
                             typename __subextents_type_impl<_IndexType,
                                                              _Tuple,
                                                              integer_sequence<size_t, _SourceExtents...>,
                                                              _Pos + 1,
                                                              _OutExtents...>::type,
                             typename __subextents_type_impl<_IndexType,
                                                              _Tuple,
                                                              integer_sequence<size_t, _SourceExtents...>,
                                                              _Pos + 1,
                                                              _OutExtents...,
                                                              (is_same_v<_Slice, full_extent_t>
                                                                   ? __source_static_extent<_Pos, _SourceExtents...>
                                                                   : __static_subextent<_Slice>::value)>::type>;
};

template <class _IndexType, class _Tuple, size_t _Pos, size_t _OutPos, size_t... _SourceExtents>
_LIBCPP_HIDE_FROM_ABI constexpr void __subextents_values(const _Tuple& __slices,
                                                         const extents<_IndexType, _SourceExtents...>& __src,
                                                         array<_IndexType, sizeof...(_SourceExtents)>& __values) {
  if constexpr (_Pos < sizeof...(_SourceExtents)) {
    using _Slice = remove_cvref_t<decltype(get<_Pos>(__slices))>;
    if constexpr (__extent_slice<_Slice> || is_same_v<_Slice, full_extent_t>) {
      if constexpr (is_same_v<_Slice, full_extent_t>)
        __values[_OutPos] = __src.extent(_Pos);
      else
        __values[_OutPos] = _IndexType(get<_Pos>(__slices).extent);
      __subextents_values<_IndexType, _Tuple, _Pos + 1, _OutPos + 1>(__slices, __src, __values);
    } else {
      __subextents_values<_IndexType, _Tuple, _Pos + 1, _OutPos>(__slices, __src, __values);
    }
  }
}

template <class _IndexType, size_t... _Extents, class _Tuple>
_LIBCPP_HIDE_FROM_ABI constexpr auto __subextents(const extents<_IndexType, _Extents...>& __src,
                                                  const _Tuple& __slices) {
  using _Result = typename __subextents_type_impl<_IndexType, _Tuple, integer_sequence<size_t, _Extents...>, 0>::type;
  array<_IndexType, sizeof...(_Extents)> __values{};
  __subextents_values<_IndexType, _Tuple, 0, 0>(__slices, __src, __values);
  array<_IndexType, _Result::rank()> __result_values{};
  for (size_t __i = 0; __i < _Result::rank(); ++__i)
    __result_values[__i] = __values[__i];
  return _Result(__result_values);
}
} // namespace __mdspan_detail

template <class _IndexType, size_t... _Extents, class... _SliceSpecifiers>
  requires(sizeof...(_SliceSpecifiers) == sizeof...(_Extents))
_LIBCPP_HIDE_FROM_ABI constexpr auto subextents(const extents<_IndexType, _Extents...>& __src,
                                                _SliceSpecifiers... __slices) {
  auto __canonical = canonical_slices(__src, std::forward<decltype(__slices)>(__slices)...);
  return __mdspan_detail::__subextents<_IndexType>(__src, __canonical);
}

#endif
_LIBCPP_END_NAMESPACE_STD

#endif
