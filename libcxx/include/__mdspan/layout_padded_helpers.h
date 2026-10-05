// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___MDSPAN_LAYOUT_PADDED_HELPERS_H
#define _LIBCPP___MDSPAN_LAYOUT_PADDED_HELPERS_H

#include <__config>
#include <__cstddef/size_t.h>
#include <__fwd/mdspan.h>
#include <__type_traits/is_same.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 23

namespace __mdspan_detail {

// [mdspan.layout.general]: LEAST-MULTIPLE-AT-LEAST(x, y) is y if x is zero, otherwise the least
// multiple of x that is greater than or equal to y. Returns false if the result is not
// representable as size_t.
_LIBCPP_HIDE_FROM_ABI constexpr bool
__least_multiple_at_least_checked(size_t __x, size_t __y, size_t& __result) noexcept {
  if (__x == 0) {
    __result = __y;
    return true;
  }
  const size_t __rem = __y % __x;
  if (__rem == 0) {
    __result = __y;
    return true;
  }
  return !__builtin_add_overflow(__y, __x - __rem, &__result);
}

_LIBCPP_HIDE_FROM_ABI constexpr size_t __least_multiple_at_least(size_t __x, size_t __y) noexcept {
  size_t __result = 0;
  (void)__mdspan_detail::__least_multiple_at_least_checked(__x, __y, __result);
  return __result;
}

// [mdspan.layout.general]: is-mapping-of, is-layout-left-padded-mapping-of, is-layout-right-padded-mapping-of.
template <class _Layout, class _Mapping>
inline constexpr bool __is_mapping_of = requires {
  typename _Mapping::extents_type;
  requires is_same_v<typename _Layout::template mapping<typename _Mapping::extents_type>, _Mapping>;
};

template <class _Layout>
inline constexpr bool __is_layout_left_padded_policy = false;
template <size_t _PaddingValue>
inline constexpr bool __is_layout_left_padded_policy<layout_left_padded<_PaddingValue>> = true;

template <class _Layout>
inline constexpr bool __is_layout_right_padded_policy = false;
template <size_t _PaddingValue>
inline constexpr bool __is_layout_right_padded_policy<layout_right_padded<_PaddingValue>> = true;

template <class _Mapping>
concept __is_layout_left_padded_mapping_of =
    requires {
      typename _Mapping::layout_type;
      typename _Mapping::extents_type;
    } && __is_layout_left_padded_policy<typename _Mapping::layout_type> &&
    __is_mapping_of<typename _Mapping::layout_type, _Mapping>;

template <class _Mapping>
concept __is_layout_right_padded_mapping_of =
    requires {
      typename _Mapping::layout_type;
      typename _Mapping::extents_type;
    } && __is_layout_right_padded_policy<typename _Mapping::layout_type> &&
    __is_mapping_of<typename _Mapping::layout_type, _Mapping>;

// static-padding-stride of a padded mapping type, as defined in [mdspan.layout.leftpad.expo] /
// [mdspan.layout.rightpad.expo]; `_First` selects the first (left) or last (right) static extent.
template <class _PaddedMapping, bool _First>
inline constexpr size_t __padded_static_stride = [] {
  using _Extents          = typename _PaddedMapping::extents_type;
  constexpr size_t __rank = _Extents::rank();
  if constexpr (__rank <= 1) {
    return size_t(0);
  } else {
    constexpr size_t __static_extent = _Extents::static_extent(_First ? 0 : __rank - 1);
    if constexpr (_PaddedMapping::padding_value == dynamic_extent || __static_extent == dynamic_extent)
      return dynamic_extent;
    else
      return __mdspan_detail::__least_multiple_at_least(_PaddedMapping::padding_value, __static_extent);
  }
}();

} // namespace __mdspan_detail

#endif // _LIBCPP_STD_VER >= 23

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___MDSPAN_LAYOUT_PADDED_HELPERS_H
