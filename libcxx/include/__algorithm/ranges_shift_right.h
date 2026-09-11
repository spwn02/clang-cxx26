//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___ALGORITHM_RANGES_SHIFT_RIGHT_H
#define _LIBCPP___ALGORITHM_RANGES_SHIFT_RIGHT_H

#include <__algorithm/shift_right.h>
#include <__config>
#include <__iterator/concepts.h>
#include <__iterator/next.h>
#include <__iterator/permutable.h>
#include <__ranges/access.h>
#include <__ranges/concepts.h>
#include <__ranges/subrange.h>
#include <__utility/move.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

#if _LIBCPP_STD_VER >= 23

_LIBCPP_BEGIN_NAMESPACE_STD

namespace ranges {
struct __shift_right {
  // [alg.shift]: the result denotes the range of elements that were moved into their
  // shifted position, i.e. [result-of-the-single-iterator-overload, last).
  template <class _Iter, class _Sent>
  _LIBCPP_HIDE_FROM_ABI constexpr static subrange<_Iter>
  __shift_right_fn_impl(_Iter __first, _Sent __last, iter_difference_t<_Iter> __n) {
    _Iter __real_last = ranges::next(__first, __last);
    _Iter __new_begin  = std::shift_right(__first, __real_last, __n);
    return {std::move(__new_begin), std::move(__real_last)};
  }

  template <permutable _Iter, sentinel_for<_Iter> _Sent>
  _LIBCPP_HIDE_FROM_ABI constexpr subrange<_Iter> operator()(_Iter __first, _Sent __last, iter_difference_t<_Iter> __n) const {
    return __shift_right_fn_impl(std::move(__first), std::move(__last), __n);
  }

  template <forward_range _Range>
    requires permutable<iterator_t<_Range>>
  _LIBCPP_HIDE_FROM_ABI constexpr borrowed_subrange_t<_Range> operator()(_Range&& __range, range_difference_t<_Range> __n) const {
    return __shift_right_fn_impl(ranges::begin(__range), ranges::end(__range), __n);
  }
};

inline namespace __cpo {
inline constexpr auto shift_right = __shift_right{};
} // namespace __cpo
} // namespace ranges

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_STD_VER >= 23

_LIBCPP_POP_MACROS

#endif // _LIBCPP___ALGORITHM_RANGES_SHIFT_RIGHT_H
