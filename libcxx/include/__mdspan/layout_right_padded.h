// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___MDSPAN_LAYOUT_RIGHT_PADDED_H
#define _LIBCPP___MDSPAN_LAYOUT_RIGHT_PADDED_H

#include <__assert>
#include <__config>
#include <__cstddef/size_t.h>
#include <__fwd/mdspan.h>
#include <__mdspan/extents.h>
#include <__mdspan/layout_left.h>
#include <__mdspan/layout_padded_helpers.h>
#include <__mdspan/layout_right.h>
#include <__mdspan/layout_stride.h>
#include <__type_traits/is_constructible.h>
#include <__type_traits/is_convertible.h>
#include <__type_traits/is_nothrow_constructible.h>
#include <__utility/integer_sequence.h>
#include <array>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

// [mdspan.layout.rightpad]
template <size_t _PaddingValue>
template <class _Extents>
class layout_right_padded<_PaddingValue>::mapping {
public:
  static constexpr size_t padding_value = _PaddingValue;

  using extents_type = _Extents;
  using index_type   = typename extents_type::index_type;
  using size_type    = typename extents_type::size_type;
  using rank_type    = typename extents_type::rank_type;
  using layout_type  = layout_right_padded<_PaddingValue>;

private:
  static_assert(__mdspan_detail::__is_extents<_Extents>::value,
                "layout_right_padded::mapping template argument must be a specialization of extents.");

  static constexpr size_t __rank_ = extents_type::rank();
  static constexpr size_t __last_static_extent_ = [] {
    if constexpr (__rank_ == 0)
      return dynamic_extent;
    else
      return static_cast<size_t>(extents_type::static_extent(__rank_ - 1));
  }();

  // [mdspan.layout.rightpad.expo]
  static constexpr size_t __static_padding_stride_ = [] {
    if constexpr (__rank_ <= 1)
      return size_t(0);
    else if constexpr (_PaddingValue == dynamic_extent || __last_static_extent_ == dynamic_extent)
      return dynamic_extent;
    else
      return __mdspan_detail::__least_multiple_at_least(_PaddingValue, __last_static_extent_);
  }();

  // Mandates helpers.
  static constexpr bool __static_index_space_representable() {
    size_t __prod = 1;
    for (size_t __r = 0; __r < __rank_; ++__r)
      if (__builtin_mul_overflow(__prod, static_cast<size_t>(extents_type::static_extent(__r)), &__prod))
        return false;
    return __mdspan_detail::__is_representable_as<index_type>(__prod);
  }

  static constexpr bool __static_padded_last_representable() {
    if constexpr (__rank_ > 1 && _PaddingValue != dynamic_extent && __last_static_extent_ != dynamic_extent) {
      size_t __padded = 0;
      return __mdspan_detail::__least_multiple_at_least_checked(_PaddingValue, __last_static_extent_, __padded) &&
             __mdspan_detail::__is_representable_as<index_type>(__padded);
    }
    return true;
  }

  static constexpr bool __static_padded_product_representable() {
    if constexpr (__rank_ > 1 && _PaddingValue != dynamic_extent) {
      for (size_t __r = 0; __r < __rank_; ++__r)
        if (extents_type::static_extent(__r) == dynamic_extent)
          return true;
      size_t __prod = 0;
      if (!__mdspan_detail::__least_multiple_at_least_checked(_PaddingValue, __last_static_extent_, __prod))
        return false;
      for (size_t __r = 0; __r + 1 < __rank_; ++__r)
        if (__builtin_mul_overflow(__prod, static_cast<size_t>(extents_type::static_extent(__r)), &__prod))
          return false;
      return __mdspan_detail::__is_representable_as<index_type>(__prod);
    }
    return true;
  }

  static_assert(extents_type::rank_dynamic() != 0 || __static_index_space_representable(),
                "layout_right_padded::mapping: the size of the index space of Extents() must be representable as "
                "index_type.");
  static_assert(_PaddingValue == dynamic_extent || __mdspan_detail::__is_representable_as<index_type>(_PaddingValue),
                "layout_right_padded::mapping: padding_value must be representable as index_type.");
  static_assert(__static_padded_last_representable(),
                "layout_right_padded::mapping: LEAST-MULTIPLE-AT-LEAST(padding_value, last static extent) must be "
                "representable as size_t and index_type.");
  static_assert(__static_padded_product_representable(),
                "layout_right_padded::mapping: the padded size of the static index space must be representable as "
                "size_t and index_type.");

  // Expects helpers (runtime).
  _LIBCPP_HIDE_FROM_ABI static constexpr bool
  __padded_size_representable(const extents_type& __ext, size_t __padding) noexcept {
    if constexpr (__rank_ <= 1) {
      return true;
    } else {
      size_t __padded = 0;
      if (!__mdspan_detail::__least_multiple_at_least_checked(
              __padding, static_cast<size_t>(__ext.extent(__rank_ - 1)), __padded) ||
          !__mdspan_detail::__is_representable_as<index_type>(__padded))
        return false;
      size_t __prod = __padded;
      for (size_t __r = 0; __r + 1 < __rank_; ++__r)
        if (__builtin_mul_overflow(__prod, static_cast<size_t>(__ext.extent(__r)), &__prod))
          return false;
      return __mdspan_detail::__is_representable_as<index_type>(__prod);
    }
  }

  _LIBCPP_HIDE_FROM_ABI static constexpr bool __index_space_representable(const extents_type& __ext) noexcept {
    size_t __prod = 1;
    for (size_t __r = 0; __r < __rank_; ++__r)
      if (__builtin_mul_overflow(__prod, static_cast<size_t>(__ext.extent(__r)), &__prod))
        return false;
    return __mdspan_detail::__is_representable_as<index_type>(__prod);
  }

  template <class _OtherMapping>
  _LIBCPP_HIDE_FROM_ABI static constexpr bool __other_span_representable(const _OtherMapping& __other) noexcept {
    return __mdspan_detail::__is_representable_as<index_type>(__other.required_span_size());
  }

  _LIBCPP_HIDE_FROM_ABI static constexpr index_type
  __stride_for(const extents_type& __ext, size_t __padding) noexcept {
    if constexpr (__rank_ <= 1)
      return index_type(0);
    else
      return static_cast<index_type>(
          __mdspan_detail::__least_multiple_at_least(__padding, static_cast<size_t>(__ext.extent(__rank_ - 1))));
  }

public:
  // [mdspan.layout.rightpad.cons], constructors
  _LIBCPP_HIDE_FROM_ABI constexpr mapping() noexcept : mapping(extents_type{}) {}
  _LIBCPP_HIDE_FROM_ABI constexpr mapping(const mapping&) noexcept = default;

  _LIBCPP_HIDE_FROM_ABI constexpr mapping(const extents_type& __ext) : __extents_(__ext) {
    _LIBCPP_ASSERT_UNCATEGORIZED(
        __index_space_representable(__ext),
        "layout_right_padded::mapping(extents): the size of the index space must be representable as index_type.");
    if constexpr (__rank_ > 1) {
      _LIBCPP_ASSERT_UNCATEGORIZED(
          __padded_size_representable(__ext, _PaddingValue == dynamic_extent ? 0 : _PaddingValue),
          "layout_right_padded::mapping(extents): the padded size must be representable as index_type.");
      __stride_rm2_ = __stride_for(__ext, _PaddingValue == dynamic_extent ? 0 : _PaddingValue);
    }
  }

  template <class _OtherIndexType>
    requires(is_convertible_v<_OtherIndexType, index_type> && is_nothrow_constructible_v<index_type, _OtherIndexType>)
  _LIBCPP_HIDE_FROM_ABI constexpr mapping(const extents_type& __ext, _OtherIndexType __padding) : __extents_(__ext) {
    if constexpr (integral<_OtherIndexType>)
      _LIBCPP_ASSERT_UNCATEGORIZED(
          __mdspan_detail::__is_representable_as<index_type>(__padding) && __padding > 0,
          "layout_right_padded::mapping(extents, padding): padding must be representable as index_type and positive.");
    const index_type __pad = static_cast<index_type>(__padding);
    _LIBCPP_ASSERT_UNCATEGORIZED(
        __index_space_representable(__ext),
        "layout_right_padded::mapping(extents, padding): the size of the index space must be representable.");
    _LIBCPP_ASSERT_UNCATEGORIZED(
        _PaddingValue == dynamic_extent || static_cast<size_t>(__pad) == _PaddingValue,
        "layout_right_padded::mapping(extents, padding): padding must equal padding_value.");
    if constexpr (__rank_ > 1) {
      _LIBCPP_ASSERT_UNCATEGORIZED(
          __padded_size_representable(__ext, static_cast<size_t>(__pad)),
          "layout_right_padded::mapping(extents, padding): the padded size must be representable as index_type.");
      __stride_rm2_ = __stride_for(__ext, static_cast<size_t>(__pad));
    }
  }

  template <class _OtherExtents>
    requires(is_constructible_v<extents_type, _OtherExtents>)
  _LIBCPP_HIDE_FROM_ABI constexpr explicit(!is_convertible_v<_OtherExtents, extents_type>)
      mapping(const layout_right::mapping<_OtherExtents>& __other)
      : mapping(extents_type(__other.extents())) {
    if constexpr (_OtherExtents::rank() > 1)
      static_assert(__static_padding_stride_ == dynamic_extent ||
                        _OtherExtents::static_extent(_OtherExtents::rank() - 1) == dynamic_extent ||
                        __static_padding_stride_ == _OtherExtents::static_extent(_OtherExtents::rank() - 1),
                    "layout_right_padded::mapping from layout_right: static padding stride must equal the last "
                    "static extent.");
    if constexpr (__rank_ > 1 && _PaddingValue != dynamic_extent)
      _LIBCPP_ASSERT_UNCATEGORIZED(
          static_cast<size_t>(__other.stride(__rank_ - 2)) ==
              __mdspan_detail::__least_multiple_at_least(
                  _PaddingValue, static_cast<size_t>(__other.extents().extent(__rank_ - 1))),
          "layout_right_padded::mapping from layout_right: other.stride(rank - 2) must equal the padded last extent.");
    _LIBCPP_ASSERT_UNCATEGORIZED(
        __other_span_representable(__other),
        "layout_right_padded::mapping from layout_right: other.required_span_size() must be representable.");
  }

  template <class _OtherExtents>
    requires(is_constructible_v<extents_type, _OtherExtents>)
  _LIBCPP_HIDE_FROM_ABI constexpr explicit(!(__rank_ == 0 && is_convertible_v<_OtherExtents, extents_type>))
      mapping(const layout_stride::mapping<_OtherExtents>& __other)
      : __extents_(__other.extents()) {
    if constexpr (__rank_ > 1 && _PaddingValue != dynamic_extent)
      _LIBCPP_ASSERT_UNCATEGORIZED(
          static_cast<size_t>(__other.stride(__rank_ - 2)) ==
              __mdspan_detail::__least_multiple_at_least(
                  _PaddingValue, static_cast<size_t>(__other.extents().extent(__rank_ - 1))),
          "layout_right_padded::mapping from layout_stride: other.stride(rank - 2) must equal the padded last "
          "extent.");
    if constexpr (__rank_ > 0)
      _LIBCPP_ASSERT_UNCATEGORIZED(
          __other.stride(__rank_ - 1) == 1,
          "layout_right_padded::mapping from layout_stride: other.stride(rank - 1) must be 1.");
    if constexpr (__rank_ > 2)
      _LIBCPP_ASSERT_UNCATEGORIZED(
          ([&]() {
            // rev-prod-of-extents(r) / extent(rank - 1) is the product of extent(k) for k in [r, rank - 1).
            for (size_t __r = 0; __r + 2 < __rank_; ++__r) {
              size_t __prod = 1;
              for (size_t __k = __r; __k + 1 < __rank_; ++__k)
                __prod *= static_cast<size_t>(__other.extents().extent(__k));
              if (static_cast<size_t>(__other.stride(__r)) != __prod * static_cast<size_t>(__other.stride(__rank_ - 2)))
                return false;
            }
            return true;
          }()),
          "layout_right_padded::mapping from layout_stride: strides are not compatible with layout_right_padded.");
    _LIBCPP_ASSERT_UNCATEGORIZED(
        __other_span_representable(__other),
        "layout_right_padded::mapping from layout_stride: other.required_span_size() must be representable.");
    if constexpr (__rank_ > 1)
      __stride_rm2_ = static_cast<index_type>(__other.stride(__rank_ - 2));
  }

  template <class _LayoutRightPaddedMapping>
    requires(__mdspan_detail::__is_layout_right_padded_mapping_of<_LayoutRightPaddedMapping> &&
             is_constructible_v<extents_type, typename _LayoutRightPaddedMapping::extents_type>)
  _LIBCPP_HIDE_FROM_ABI constexpr explicit(
      !is_convertible_v<typename _LayoutRightPaddedMapping::extents_type, extents_type> ||
      (__rank_ > 1 && (_PaddingValue != dynamic_extent || _LayoutRightPaddedMapping::padding_value == dynamic_extent)))
      mapping(const _LayoutRightPaddedMapping& __other)
      : __extents_(__other.extents()) {
    if constexpr (__rank_ > 1)
      static_assert(_PaddingValue == dynamic_extent || _LayoutRightPaddedMapping::padding_value == dynamic_extent ||
                        _PaddingValue == _LayoutRightPaddedMapping::padding_value,
                    "layout_right_padded::mapping: incompatible padding values.");
    if constexpr (__rank_ > 1 && _PaddingValue != dynamic_extent)
      _LIBCPP_ASSERT_UNCATEGORIZED(
          static_cast<size_t>(__other.stride(__rank_ - 2)) ==
              __mdspan_detail::__least_multiple_at_least(
                  _PaddingValue, static_cast<size_t>(__other.extents().extent(__rank_ - 1))),
          "layout_right_padded::mapping from padded mapping: other.stride(rank - 2) must equal the padded last "
          "extent.");
    _LIBCPP_ASSERT_UNCATEGORIZED(
        __other_span_representable(__other),
        "layout_right_padded::mapping from padded mapping: other.required_span_size() must be representable.");
    if constexpr (__rank_ > 1)
      __stride_rm2_ = static_cast<index_type>(__other.stride(__rank_ - 2));
  }

  template <class _LayoutLeftPaddedMapping>
    requires((__mdspan_detail::__is_layout_left_padded_mapping_of<_LayoutLeftPaddedMapping> ||
              __mdspan_detail::__is_mapping_of<layout_left, _LayoutLeftPaddedMapping>) &&
             __rank_ <= 1 && is_constructible_v<extents_type, typename _LayoutLeftPaddedMapping::extents_type>)
  _LIBCPP_HIDE_FROM_ABI constexpr explicit(!is_convertible_v<typename _LayoutLeftPaddedMapping::extents_type, extents_type>)
      mapping(const _LayoutLeftPaddedMapping& __other) noexcept
      : __extents_(__other.extents()) {
    _LIBCPP_ASSERT_UNCATEGORIZED(
        __other_span_representable(__other),
        "layout_right_padded::mapping from left mapping: other.required_span_size() must be representable.");
  }

  _LIBCPP_HIDE_FROM_ABI constexpr mapping& operator=(const mapping&) noexcept = default;

  // [mdspan.layout.rightpad.obs], observers
  _LIBCPP_HIDE_FROM_ABI constexpr const extents_type& extents() const noexcept { return __extents_; }

  _LIBCPP_HIDE_FROM_ABI constexpr array<index_type, __rank_> strides() const noexcept {
    return [&]<size_t... _Pos>(index_sequence<_Pos...>) {
      return array<index_type, __rank_>{stride(_Pos)...};
    }(make_index_sequence<__rank_>());
  }

  _LIBCPP_HIDE_FROM_ABI constexpr index_type required_span_size() const noexcept {
    for (size_t __r = 0; __r < __rank_; ++__r)
      if (__extents_.extent(__r) == 0)
        return 0;
    return [&]<size_t... _Pos>(index_sequence<_Pos...>) {
      return (*this)(static_cast<index_type>(__extents_.extent(_Pos) - index_type(1))...) + index_type(1);
    }(make_index_sequence<__rank_>());
  }

  template <class... _Indices>
    requires(sizeof...(_Indices) == __rank_ && (is_convertible_v<_Indices, index_type> && ...) &&
             (is_nothrow_constructible_v<index_type, _Indices> && ...))
  _LIBCPP_HIDE_FROM_ABI constexpr index_type operator()(_Indices... __idxs) const noexcept {
    _LIBCPP_ASSERT_UNCATEGORIZED(__mdspan_detail::__is_multidimensional_index_in(__extents_, __idxs...),
                                 "layout_right_padded::mapping: out of bounds indexing");
    return [&]<size_t... _Pos>(index_sequence<_Pos...>) {
      return ((static_cast<index_type>(__idxs) * stride(_Pos)) + ... + index_type(0));
    }(make_index_sequence<__rank_>());
  }

  _LIBCPP_HIDE_FROM_ABI static constexpr bool is_always_unique() noexcept { return true; }
  _LIBCPP_HIDE_FROM_ABI static constexpr bool is_always_exhaustive() noexcept {
    if constexpr (__rank_ <= 1)
      return true;
    else if constexpr (__static_padding_stride_ != dynamic_extent && __last_static_extent_ != dynamic_extent)
      return __static_padding_stride_ == __last_static_extent_;
    else
      return false;
  }
  _LIBCPP_HIDE_FROM_ABI static constexpr bool is_always_strided() noexcept { return true; }

  _LIBCPP_HIDE_FROM_ABI static constexpr bool is_unique() noexcept { return true; }
  _LIBCPP_HIDE_FROM_ABI constexpr bool is_exhaustive() const noexcept {
    if constexpr (__rank_ <= 1)
      return true;
    else
      return __extents_.extent(__rank_ - 1) == stride(__rank_ - 2);
  }
  _LIBCPP_HIDE_FROM_ABI static constexpr bool is_strided() noexcept { return true; }

  _LIBCPP_HIDE_FROM_ABI constexpr index_type stride(rank_type __r) const noexcept {
    _LIBCPP_ASSERT_UNCATEGORIZED(__r < __rank_, "layout_right_padded::mapping::stride(): invalid rank index");
    if (__r == __rank_ - 1)
      return index_type(1);
    index_type __s = __stride_rm2_;
    for (rank_type __k = __r + 1; __k + 1 < __rank_; ++__k)
      __s *= __extents_.extent(__k);
    return __s;
  }

  template <class _LayoutRightPaddedMapping>
    requires(__mdspan_detail::__is_layout_right_padded_mapping_of<_LayoutRightPaddedMapping> &&
             _LayoutRightPaddedMapping::extents_type::rank() == __rank_)
  _LIBCPP_HIDE_FROM_ABI friend constexpr bool
  operator==(const mapping& __x, const _LayoutRightPaddedMapping& __y) noexcept {
    if (!(__x.extents() == __y.extents()))
      return false;
    if constexpr (__rank_ < 2)
      return true;
    else
      return __x.stride(__rank_ - 2) == static_cast<index_type>(__y.stride(__rank_ - 2));
  }

private:
#  if _LIBCPP_STD_VER >= 26
  template <class... _SliceSpecifiers>
  _LIBCPP_HIDE_FROM_ABI constexpr auto __submdspan_mapping_impl(_SliceSpecifiers... __slices) const;

  template <class... _SliceSpecifiers>
    requires(sizeof...(_SliceSpecifiers) == extents_type::rank())
  _LIBCPP_HIDE_FROM_ABI friend constexpr auto submdspan_mapping(const mapping& __src, _SliceSpecifiers... __slices) {
    return __src.__submdspan_mapping_impl(__slices...);
  }

#  endif
  // [mdspan.layout.rightpad.expo]
  index_type __stride_rm2_ = static_cast<index_type>(__static_padding_stride_);
  _LIBCPP_NO_UNIQUE_ADDRESS extents_type __extents_{};
};

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP___MDSPAN_LAYOUT_RIGHT_PADDED_H
