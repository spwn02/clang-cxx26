//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___ALGORITHM_RANGES_FILL_H
#define _LIBCPP___ALGORITHM_RANGES_FILL_H

#include <__algorithm/ranges_fill_n.h>
#include <__config>
#include <__iterator/concepts.h>
#include <__iterator/iterator_traits.h>
#include <__ranges/access.h>
#include <__ranges/concepts.h>
#include <__ranges/dangling.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

#if _LIBCPP_STD_VER >= 20

_LIBCPP_BEGIN_NAMESPACE_STD

namespace ranges {
struct __fill {
  template <class _Iter,
            sentinel_for<_Iter> _Sent,
            class _Type
#  if _LIBCPP_STD_VER >= 26
            = iter_value_t<_Iter>
#  endif
            >
    requires output_iterator<_Iter, const _Type&>
  _LIBCPP_HIDE_FROM_ABI constexpr _Iter operator()(_Iter __first, _Sent __last, const _Type& __value) const {
    if constexpr (random_access_iterator<_Iter> && sized_sentinel_for<_Sent, _Iter>) {
      return ranges::fill_n(__first, __last - __first, __value);
    } else {
      for (; __first != __last; ++__first)
        *__first = __value;
      return __first;
    }
  }

  template <class _Range,
            class _Type
#  if _LIBCPP_STD_VER >= 26
            = range_value_t<_Range>
#  endif
            >
    requires output_range<_Range, const _Type&>
  _LIBCPP_HIDE_FROM_ABI constexpr borrowed_iterator_t<_Range> operator()(_Range&& __range, const _Type& __value) const {
    return (*this)(ranges::begin(__range), ranges::end(__range), __value);
  }
};

inline namespace __cpo {
inline constexpr auto fill = __fill{};
} // namespace __cpo
} // namespace ranges

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_STD_VER >= 20

_LIBCPP_POP_MACROS

#endif // _LIBCPP___ALGORITHM_RANGES_FILL_H
