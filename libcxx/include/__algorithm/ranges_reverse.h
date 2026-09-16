//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___ALGORITHM_RANGES_REVERSE_H
#define _LIBCPP___ALGORITHM_RANGES_REVERSE_H

#include <__config>
#include <__algorithm/pstl.h>
#include <__type_traits/is_execution_policy.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/forward.h>
#include <__iterator/concepts.h>
#include <__iterator/iter_swap.h>
#include <__iterator/next.h>
#include <__iterator/permutable.h>
#include <__ranges/access.h>
#include <__ranges/concepts.h>
#include <__ranges/dangling.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_STD_VER >= 20

_LIBCPP_BEGIN_NAMESPACE_STD

namespace ranges {
struct __reverse {
  template <bidirectional_iterator _Iter, sentinel_for<_Iter> _Sent>
    requires permutable<_Iter>
  _LIBCPP_HIDE_FROM_ABI constexpr _Iter operator()(_Iter __first, _Sent __last) const {
    if constexpr (random_access_iterator<_Iter>) {
      if (__first == __last)
        return __first;

      auto __end = ranges::next(__first, __last);
      auto __ret = __end;

      while (__first < --__end) {
        ranges::iter_swap(__first, __end);
        ++__first;
      }
      return __ret;
    } else {
      auto __end = ranges::next(__first, __last);
      auto __ret = __end;

      while (__first != __end) {
        if (__first == --__end)
          break;

        ranges::iter_swap(__first, __end);
        ++__first;
      }
      return __ret;
    }
  }

  template <bidirectional_range _Range>
    requires permutable<iterator_t<_Range>>
  _LIBCPP_HIDE_FROM_ABI constexpr borrowed_iterator_t<_Range> operator()(_Range&& __range) const {
    return (*this)(ranges::begin(__range), ranges::end(__range));
  }
#  if _LIBCPP_HAS_EXPERIMENTAL_PSTL
  template <class _Ep, random_access_iterator _Iter, sized_sentinel_for<_Iter> _Sent,
            class _RawPolicy = __remove_cvref_t<_Ep>, enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
  _LIBCPP_HIDE_FROM_ABI _Iter operator()(_Ep&& __exec, _Iter __first, _Sent __last) const {
    _Iter __end = __first + (__last - __first);
    std::reverse(std::forward<_Ep>(__exec), __first, __end);
    return __end;
  }
  template <class _Ep, random_access_range _Range, class _RawPolicy = __remove_cvref_t<_Ep>,
            enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires sized_range<_Range>
  _LIBCPP_HIDE_FROM_ABI borrowed_iterator_t<_Range> operator()(_Ep&& __exec, _Range&& __range) const {
    return (*this)(std::forward<_Ep>(__exec), ranges::begin(__range), ranges::end(__range));
  }
#  endif
};

inline namespace __cpo {
inline constexpr auto reverse = __reverse{};
} // namespace __cpo
} // namespace ranges

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_STD_VER >= 20

#endif // _LIBCPP___ALGORITHM_RANGES_REVERSE_H
