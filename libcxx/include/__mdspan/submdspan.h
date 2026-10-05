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

#include <__assert>
#include <__concepts/convertible_to.h>
#include <__concepts/equality_comparable.h>
#include <__config>
#include <__mdspan/extents.h>
#include <__type_traits/conditional.h>
#include <__type_traits/integer_traits.h>
#include <__type_traits/is_convertible.h>
#include <__type_traits/is_integral.h>
#include <__type_traits/is_same.h>
#include <__type_traits/is_signed.h>
#include <__type_traits/remove_cv.h>
#include <__type_traits/remove_cvref.h>
#include <__type_traits/type_identity.h>
#include <__utility/constant_wrapper.h>
#include <__utility/forward.h>
#include <__utility/integer_sequence.h>
#include <array>
#include <tuple>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

namespace __mdspan_detail {
// [expr.const]: integral-constant-like.
template <class _Tp>
concept __mdspan_integral_constant_like =
    is_integral_v<remove_cvref_t<decltype(_Tp::value)>> && !is_same_v<bool, remove_cvref_t<decltype(_Tp::value)>> &&
    convertible_to<_Tp, const decltype(_Tp::value)&> && equality_comparable_with<_Tp, decltype(_Tp::value)> &&
    bool_constant<_Tp() == _Tp::value>::value &&
    bool_constant<static_cast<decltype(_Tp::value)>(_Tp()) == _Tp::value>::value;

// [mdspan.sub.range.slices]: the member types of extent_slice and range_slice are signed or unsigned integer types or
// model integral-constant-like.
template <class _Tp>
inline constexpr bool __valid_slice_member_type = __signed_or_unsigned_integer<remove_cv_t<_Tp>> || __mdspan_integral_constant_like<_Tp>;
} // namespace __mdspan_detail

struct full_extent_t {
  explicit constexpr full_extent_t() = default;
};
inline constexpr full_extent_t full_extent{};

template <class _OffsetType, class _ExtentType, class _StrideType>
struct extent_slice {
  static_assert(__mdspan_detail::__valid_slice_member_type<_OffsetType> &&
                    __mdspan_detail::__valid_slice_member_type<_ExtentType> &&
                    __mdspan_detail::__valid_slice_member_type<_StrideType>,
                "extent_slice: OffsetType, ExtentType and StrideType must be signed or unsigned integer types or "
                "model integral-constant-like");

  using offset_type = _OffsetType;
  using extent_type = _ExtentType;
  using stride_type = _StrideType;
  [[no_unique_address]] offset_type offset{};
  [[no_unique_address]] extent_type extent{};
  [[no_unique_address]] stride_type stride{};
};

template <class _FirstType, class _LastType, class _StrideType = constant_wrapper<1zu>>
struct range_slice {
  static_assert(__mdspan_detail::__valid_slice_member_type<_FirstType> &&
                    __mdspan_detail::__valid_slice_member_type<_LastType> &&
                    __mdspan_detail::__valid_slice_member_type<_StrideType>,
                "range_slice: FirstType, LastType and StrideType must be signed or unsigned integer types or "
                "model integral-constant-like");

  [[no_unique_address]] _FirstType first{};
  [[no_unique_address]] _LastType last{};
  [[no_unique_address]] _StrideType stride{};
};

namespace __mdspan_detail {
template <class _Tp>
inline constexpr bool __is_constant_wrapper = false;
template <auto _Value, class _Type>
inline constexpr bool __is_constant_wrapper<constant_wrapper<_Value, _Type>> = true;

template <class _Tp>
inline constexpr bool __is_extent_slice = false;
template <class _OffsetType, class _ExtentType, class _StrideType>
inline constexpr bool __is_extent_slice<extent_slice<_OffsetType, _ExtentType, _StrideType>> = true;
template <class _Tp>
concept __extent_slice = __is_extent_slice<remove_cvref_t<_Tp>>;

template <class _Tp>
inline constexpr bool __is_range_slice = false;
template <class _FirstType, class _LastType, class _StrideType>
inline constexpr bool __is_range_slice<range_slice<_FirstType, _LastType, _StrideType>> = true;
template <class _Tp>
concept __range_slice = __is_range_slice<remove_cvref_t<_Tp>>;

// [mdspan.sub.overview]: a collapsing slice type is neither full_extent_t nor a specialization of extent_slice.
template <class _Slice>
inline constexpr bool __is_collapsing_slice = !is_same_v<remove_cvref_t<_Slice>, full_extent_t> && !__is_extent_slice<remove_cvref_t<_Slice>>;

template <class _Value>
_LIBCPP_HIDE_FROM_ABI constexpr bool __is_negative(_Value __v) noexcept {
  if constexpr (is_signed_v<_Value>)
    return __v < 0;
  else
    return false;
}

// "is representable as a value of type _To"
template <class _To, class _From>
_LIBCPP_HIDE_FROM_ABI constexpr bool __representable(_From __v) noexcept {
  const _To __r = static_cast<_To>(__v);
  return static_cast<_From>(__r) == __v && __is_negative(__r) == __is_negative(__v);
}

// [mdspan.sub.overview]: submdspan slice type for IndexType.
template <class _Slice, class _IndexType>
inline constexpr bool __is_extent_slice_for = false;
template <class _OffsetType, class _ExtentType, class _StrideType, class _IndexType>
inline constexpr bool __is_extent_slice_for<extent_slice<_OffsetType, _ExtentType, _StrideType>, _IndexType> =
    is_convertible_v<_OffsetType, _IndexType> && is_convertible_v<_ExtentType, _IndexType> &&
    is_convertible_v<_StrideType, _IndexType>;

template <class _Slice, class _IndexType>
inline constexpr bool __is_range_slice_for = false;
template <class _FirstType, class _LastType, class _StrideType, class _IndexType>
inline constexpr bool __is_range_slice_for<range_slice<_FirstType, _LastType, _StrideType>, _IndexType> =
    is_convertible_v<_FirstType, _IndexType> && is_convertible_v<_LastType, _IndexType> &&
    is_convertible_v<_StrideType, _IndexType>;

// The element types of a two-element structured binding are checked where the binding is made (__canonical_slice).
template <class _Slice>
concept __two_element_slice = requires { requires __builtin_structured_binding_size(_Slice) == 2; };

template <class _Slice, class _IndexType>
inline constexpr bool __is_submdspan_slice_type =
    is_convertible_v<_Slice, full_extent_t> || is_convertible_v<_Slice, _IndexType> ||
    __is_extent_slice_for<_Slice, _IndexType> || __is_range_slice_for<_Slice, _IndexType> ||
    __two_element_slice<_Slice>;

// [mdspan.sub.overview]: canonical submdspan index type for IndexType.
template <class _Slice, class _IndexType>
consteval bool __is_canonical_index_type() {
  if constexpr (is_same_v<_Slice, _IndexType>) {
    return true;
  } else if constexpr (__is_constant_wrapper<_Slice>) {
    return is_same_v<remove_cvref_t<decltype(_Slice::value)>, _IndexType> && !__is_negative(_Slice::value);
  } else {
    return false;
  }
}

// [mdspan.sub.overview]: canonical submdspan slice type for IndexType.
template <class _Slice, class _IndexType>
consteval bool __is_canonical_slice_type() {
  if constexpr (is_same_v<_Slice, full_extent_t>) {
    return true;
  } else if constexpr (__is_canonical_index_type<_Slice, _IndexType>()) {
    return true;
  } else if constexpr (__is_extent_slice<_Slice>) {
    if constexpr (!(__is_canonical_index_type<typename _Slice::offset_type, _IndexType>() &&
                    __is_canonical_index_type<typename _Slice::extent_type, _IndexType>() &&
                    __is_canonical_index_type<typename _Slice::stride_type, _IndexType>())) {
      return false;
    } else if constexpr (__is_constant_wrapper<typename _Slice::stride_type> &&
                         __is_constant_wrapper<typename _Slice::extent_type>) {
      return _Slice::stride_type::value > 0;
    } else {
      return true;
    }
  } else {
    return false;
  }
}

template <class _Tp, unsigned long long _Default>
consteval unsigned long long __constant_value_or() {
  if constexpr (__is_constant_wrapper<_Tp>)
    return static_cast<unsigned long long>(_Tp::value);
  else
    return _Default;
}

// [mdspan.sub.overview]: valid submdspan slice type for the kth extent of an extents whose static extent is _Static.
template <class _Slice, class _IndexType, size_t _Static>
consteval bool __is_valid_slice_type_for_static_extent() {
  if constexpr (!__is_canonical_slice_type<_Slice, _IndexType>()) {
    return false;
  } else if constexpr (_Static == dynamic_extent) {
    return true;
  } else if constexpr (__is_extent_slice<_Slice>) {
    using _Offset = typename _Slice::offset_type;
    using _Extent = typename _Slice::extent_type;
    using _Stride = typename _Slice::stride_type;
    constexpr unsigned long long __o = __constant_value_or<_Offset, 0>();
    constexpr unsigned long long __e = __constant_value_or<_Extent, 0>();
    constexpr unsigned long long __t = __constant_value_or<_Stride, 1>();
    if (__o > _Static || __e > _Static)
      return false;
    if (__e > 1 && !(__t > 0))
      return false;
    if (__e > 0) {
      unsigned long long __product = 0;
      unsigned long long __sum     = 0;
      if (__builtin_mul_overflow(__e - 1, __t, &__product) || __builtin_add_overflow(__o + 1, __product, &__sum))
        return false;
      return __sum <= _Static;
    }
    return true;
  } else if constexpr (__is_constant_wrapper<_Slice>) {
    return static_cast<unsigned long long>(_Slice::value) < _Static;
  } else {
    return true;
  }
}

// Every _Slices... is a valid submdspan slice type for the corresponding extent of _Extents.
template <class _Extents, class... _Slices>
inline constexpr bool __are_valid_slice_types = [] {
  if constexpr (sizeof...(_Slices) != _Extents::rank()) {
    return false;
  } else {
    return []<size_t... _Ks>(index_sequence<_Ks...>) {
      return (true && ... &&
              __is_valid_slice_type_for_static_extent<_Slices, typename _Extents::index_type, _Extents::static_extent(_Ks)>());
    }(make_index_sequence<sizeof...(_Slices)>{});
  }
}();

// Whether the runtime canonical slice __s is a valid submdspan slice for an extent of size __extent
// ([mdspan.sub.overview]); the slice type is already known to be a valid slice type.
template <class _IndexType, class _Slice>
_LIBCPP_HIDE_FROM_ABI constexpr bool __is_valid_slice(const _Slice& __s, _IndexType __extent) noexcept {
  if constexpr (is_same_v<_Slice, full_extent_t>) {
    return true;
  } else if constexpr (__is_extent_slice<_Slice>) {
    const _IndexType __o = static_cast<_IndexType>(__s.offset);
    const _IndexType __e = static_cast<_IndexType>(__s.extent);
    const _IndexType __t = static_cast<_IndexType>(__s.stride);
    if (__is_negative(__o) || __is_negative(__e) || (__e >= 2 && !(__t > 0)))
      return false;
    unsigned long long __upper = static_cast<unsigned long long>(__o);
    if (__e != 0) {
      unsigned long long __product = 0;
      if (__builtin_mul_overflow(static_cast<unsigned long long>(__e - 1), static_cast<unsigned long long>(__t), &__product) ||
          __builtin_add_overflow(__upper + 1, __product, &__upper))
        return false;
    }
    return __upper <= static_cast<unsigned long long>(__extent);
  } else {
    const _IndexType __i = static_cast<_IndexType>(__s);
    return !__is_negative(__i) && static_cast<unsigned long long>(__i) < static_cast<unsigned long long>(__extent);
  }
}

template <class _IndexType, size_t... _Extents, class... _Slices>
_LIBCPP_HIDE_FROM_ABI constexpr bool
__are_valid_slices(const extents<_IndexType, _Extents...>& __src, const _Slices&... __slices) noexcept {
  return [&]<size_t... _Ks>(index_sequence<_Ks...>) {
    return (true && ... && __is_valid_slice<_IndexType>(__slices, __src.extent(_Ks)));
  }(make_index_sequence<sizeof...(_Slices)>{});
}

// [mdspan.sub.helpers]
template <class _IndexType, class _Slice>
_LIBCPP_HIDE_FROM_ABI constexpr auto __canonical_index(_Slice __s) {
  if constexpr (__mdspan_integral_constant_like<_Slice>) {
    static_assert(__representable<_IndexType>(_Slice::value),
                  "submdspan: a constant slice index must be representable as a value of the extents' index_type");
    return constant_wrapper<_IndexType(_Slice::value)>{};
  } else {
    if constexpr (is_integral_v<_Slice> && !is_same_v<bool, _Slice>)
      _LIBCPP_ASSERT_UNCATEGORIZED(
          __representable<_IndexType>(__s), "submdspan: a slice index must be representable as a value of the index_type");
    return static_cast<_IndexType>(static_cast<_Slice&&>(__s));
  }
}

template <class _IndexType, class _SpanType, class... _StrideTypes>
struct __range_stride;
template <class _IndexType, class _SpanType>
struct __range_stride<_IndexType, _SpanType> {
  using type = constant_wrapper<_IndexType(1)>;
};
template <class _IndexType, class _SpanType, class _StrideType>
struct __range_stride<_IndexType, _SpanType, _StrideType> {
  using type = conditional_t<is_same_v<_SpanType, constant_wrapper<_IndexType(0)>>,
                             constant_wrapper<_IndexType(1)>,
                             _StrideType>;
};

template <class _IndexType, class _OffsetType, class _SpanType, class... _StrideTypes>
_LIBCPP_HIDE_FROM_ABI constexpr auto
__canonical_slice_range(_OffsetType __offset, _SpanType __span, _StrideTypes... __strides) {
  static_assert(sizeof...(_StrideTypes) <= 1);
  using _StrideType = typename __range_stride<_IndexType, _SpanType, _StrideTypes...>::type;
  if constexpr (__is_constant_wrapper<_StrideType>)
    static_assert(_StrideType::value > 0, "submdspan: the stride of a range slice must be greater than zero");

  const _IndexType __span_value = static_cast<_IndexType>(__span);
  auto __stride                 = [&] {
    if constexpr (__is_constant_wrapper<_StrideType>)
      return _StrideType();
    else if (__span_value == 0)
      return _IndexType(1);
    else
      return _IndexType(__strides...[0]);
  }();
  const _IndexType __stride_value = static_cast<_IndexType>(__stride);
  if constexpr (!__is_constant_wrapper<_StrideType>)
    _LIBCPP_ASSERT_UNCATEGORIZED(__stride_value > 0, "submdspan: the stride of a range slice must be greater than zero");

  if constexpr (__is_constant_wrapper<_SpanType> && __is_constant_wrapper<_StrideType>) {
    constexpr _IndexType __extent_value =
        _SpanType::value != 0 ? static_cast<_IndexType>(1 + (_SpanType::value - 1) / _StrideType::value) : _IndexType(0);
    return extent_slice<_OffsetType, constant_wrapper<__extent_value>, decltype(__stride)>{
        __offset, constant_wrapper<__extent_value>{}, __stride};
  } else {
    const _IndexType __extent_value =
        __span_value != 0 ? static_cast<_IndexType>(1 + (__span_value - 1) / __stride_value) : _IndexType(0);
    return extent_slice<_OffsetType, _IndexType, decltype(__stride)>{__offset, __extent_value, __stride};
  }
}

template <class _IndexType, class _Slice>
_LIBCPP_HIDE_FROM_ABI constexpr auto __canonical_slice(_Slice __s) {
  static_assert(__is_submdspan_slice_type<_Slice, _IndexType>,
                "submdspan: a slice specifier must be a submdspan slice type for the extents' index_type");
  if constexpr (is_convertible_v<_Slice, full_extent_t>) {
    return static_cast<full_extent_t>(static_cast<_Slice&&>(__s));
  } else if constexpr (is_convertible_v<_Slice, _IndexType>) {
    return __canonical_index<_IndexType>(static_cast<_Slice&&>(__s));
  } else if constexpr (__extent_slice<_Slice>) {
    return extent_slice{__canonical_index<_IndexType>(static_cast<typename _Slice::offset_type&&>(__s.offset)),
                        __canonical_index<_IndexType>(static_cast<typename _Slice::extent_type&&>(__s.extent)),
                        __canonical_index<_IndexType>(static_cast<typename _Slice::stride_type&&>(__s.stride))};
  } else if constexpr (__range_slice<_Slice>) {
    auto __first = __canonical_index<_IndexType>(static_cast<decltype(_Slice::first)&&>(__s.first));
    auto __last  = __canonical_index<_IndexType>(static_cast<decltype(_Slice::last)&&>(__s.last));
    return __canonical_slice_range<_IndexType>(
        __first,
        __canonical_index<_IndexType>(__last - __first),
        __canonical_index<_IndexType>(static_cast<decltype(_Slice::stride)&&>(__s.stride)));
  } else if constexpr (__two_element_slice<_Slice>) {
    auto [__s_first, __s_last] = static_cast<_Slice&&>(__s);
    static_assert(is_convertible_v<decltype(static_cast<decltype(__s_first)&&>(__s_first)), _IndexType> &&
                      is_convertible_v<decltype(static_cast<decltype(__s_last)&&>(__s_last)), _IndexType>,
                  "submdspan: the elements of a two-element slice must be convertible to the extents' index_type");
    auto __first = __canonical_index<_IndexType>(static_cast<decltype(__s_first)&&>(__s_first));
    auto __last  = __canonical_index<_IndexType>(static_cast<decltype(__s_last)&&>(__s_last));
    return __canonical_slice_range<_IndexType>(__first, __canonical_index<_IndexType>(__last - __first));
  } else {
    // Unreachable once the Mandates hold; keeps the diagnostics above from cascading.
    return _IndexType();
  }
}
} // namespace __mdspan_detail

// [mdspan.sub.canonical]
template <class _IndexType, size_t... _Extents, class... _SliceSpecifiers>
  requires(sizeof...(_SliceSpecifiers) == sizeof...(_Extents))
_LIBCPP_HIDE_FROM_ABI constexpr auto
canonical_slices(const extents<_IndexType, _Extents...>& __src, _SliceSpecifiers... __slices) {
  static_assert((__mdspan_detail::__is_submdspan_slice_type<_SliceSpecifiers, _IndexType> && ... && true),
                "canonical_slices: every slice specifier must be a submdspan slice type for the index_type");
  static_assert(
      __mdspan_detail::__are_valid_slice_types<extents<_IndexType, _Extents...>,
                                               decltype(__mdspan_detail::__canonical_slice<_IndexType>(
                                                   static_cast<_SliceSpecifiers&&>(__slices)))...>,
      "canonical_slices: every canonical slice must be a valid submdspan slice type for the corresponding extent");
  auto __result = std::make_tuple(__mdspan_detail::__canonical_slice<_IndexType>(static_cast<_SliceSpecifiers&&>(__slices))...);
  _LIBCPP_ASSERT_UNCATEGORIZED(
      std::apply([&](const auto&... __cs) { return __mdspan_detail::__are_valid_slices(__src, __cs...); }, __result),
      "canonical_slices: every slice must be a valid submdspan slice for the corresponding extent");
  (void)__src;
  return __result;
}

namespace __mdspan_detail {
// The static extent a canonical slice contributes to the result of subextents, given the source static extent.
template <class _Slice, size_t _SourceStatic>
consteval size_t __static_subextent() {
  if constexpr (is_same_v<_Slice, full_extent_t>) {
    return _SourceStatic;
  } else if constexpr (__is_extent_slice<_Slice>) {
    return __constant_value_or<typename _Slice::extent_type, dynamic_extent>();
  } else {
    return dynamic_extent;
  }
}

template <size_t _Size>
consteval size_t __count_surviving(const array<bool, _Size>& __collapsing) {
  size_t __count = 0;
  for (bool __c : __collapsing)
    __count += !__c;
  return __count;
}

// MAP_RANK(slices, k) for every k.
template <size_t _Size>
consteval array<size_t, _Size> __map_rank(const array<bool, _Size>& __collapsing) {
  array<size_t, _Size> __result{};
  size_t __count = 0;
  for (size_t __k = 0; __k < _Size; ++__k) {
    __result[__k] = __count;
    __count += !__collapsing[__k];
  }
  return __result;
}

template <size_t _SubRank, size_t _Size>
consteval array<size_t, _SubRank>
__sub_static_extents(const array<bool, _Size>& __collapsing, const array<size_t, _Size>& __statics) {
  array<size_t, _SubRank> __result{};
  size_t __count = 0;
  for (size_t __k = 0; __k < _Size; ++__k)
    if (!__collapsing[__k])
      __result[__count++] = __statics[__k];
  return __result;
}

template <class _IndexType, auto _Statics, size_t... _Js>
extents<_IndexType, _Statics[_Js]...> __make_subextents_type(index_sequence<_Js...>);

// The extents type that subextents returns for canonical slice types _Slices.
template <class _IndexType, class _Extents, class... _Slices>
struct __subextents_info;

template <class _IndexType, size_t... _Extents, class... _Slices>
struct __subextents_info<_IndexType, extents<_IndexType, _Extents...>, _Slices...> {
  static constexpr size_t __rank = sizeof...(_Slices);

  static constexpr array<bool, __rank> __collapsing              = {__is_collapsing_slice<_Slices>...};
  static constexpr array<size_t, __rank> __static_values         = {__static_subextent<_Slices, _Extents>()...};
  static constexpr size_t __sub_rank                             = __count_surviving(__collapsing);
  static constexpr array<size_t, __rank> __map                   = __map_rank(__collapsing);
  static constexpr array<size_t, __sub_rank> __sub_static_values = __sub_static_extents<__sub_rank>(__collapsing, __static_values);

  using type = decltype(__make_subextents_type<_IndexType, __sub_static_values>(make_index_sequence<__sub_rank>{}));
};

template <class _IndexType, size_t... _Extents, class... _Slices>
_LIBCPP_HIDE_FROM_ABI constexpr auto
__subextents_of_canonical(const extents<_IndexType, _Extents...>& __src, const _Slices&... __slices) {
  using _Info   = __subextents_info<_IndexType, extents<_IndexType, _Extents...>, _Slices...>;
  using _Result = typename _Info::type;
  if constexpr (_Result::rank() == 0) {
    (void)__src;
    ((void)__slices, ...);
    return _Result{};
  } else {
    array<_IndexType, _Result::rank()> __values{};
    [&]<size_t... _Ks>(index_sequence<_Ks...>) {
      auto __set = [&]<size_t _Kdx>(const auto& __slice) {
        using _Slice = remove_cvref_t<decltype(__slice)>;
        if constexpr (is_same_v<_Slice, full_extent_t>)
          __values[_Info::__map[_Kdx]] = __src.extent(_Kdx);
        else if constexpr (__is_extent_slice<_Slice>)
          __values[_Info::__map[_Kdx]] = static_cast<_IndexType>(__slice.extent);
      };
      (__set.template operator()<_Ks>(__slices), ...);
    }(make_index_sequence<sizeof...(_Slices)>{});
    return _Result(__values);
  }
}
} // namespace __mdspan_detail

// [mdspan.sub.extents]
template <class _IndexType, size_t... _Extents, class... _SliceSpecifiers>
  requires(sizeof...(_SliceSpecifiers) == sizeof...(_Extents))
_LIBCPP_HIDE_FROM_ABI constexpr auto
subextents(const extents<_IndexType, _Extents...>& __src, _SliceSpecifiers... __raw_slices) {
  auto __canonical = std::canonical_slices(__src, static_cast<_SliceSpecifiers&&>(__raw_slices)...);
  return std::apply(
      [&](const auto&... __slices) { return __mdspan_detail::__subextents_of_canonical(__src, __slices...); },
      __canonical);
}

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___MDSPAN_SUBMDSPAN_H
