//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___ALGORITHM_RANGES_FILL_H
#define _LIBCPP___ALGORITHM_RANGES_FILL_H

#include <__algorithm/fill.h>
#include <__algorithm/fill_n.h>
#include <__algorithm/pstl.h>
#include <__config>
#include <__iterator/concepts.h>
#include <__iterator/iterator_traits.h>
#include <__ranges/access.h>
#include <__ranges/concepts.h>
#include <__ranges/dangling.h>
#include <__type_traits/is_execution_policy.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/forward.h>
#include <__utility/move.h>

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
    if constexpr (sized_sentinel_for<_Sent, _Iter>) {
      auto __n = __last - __first;
      return std::__fill_n(std::move(__first), __n, __value);
    } else {
      return std::__fill(std::move(__first), std::move(__last), __value);
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

#  if _LIBCPP_HAS_EXPERIMENTAL_PSTL
  template <class _Ep, random_access_iterator _Iter, sized_sentinel_for<_Iter> _Sent, class _Type
#    if _LIBCPP_STD_VER >= 26
            = iter_value_t<_Iter>
#    endif
            , class _RawPolicy = __remove_cvref_t<_Ep>, enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires output_iterator<_Iter, const _Type&>
  _LIBCPP_HIDE_FROM_ABI _Iter
  operator()(_Ep&& __exec, _Iter __first, _Sent __last, const _Type& __value) const {
    _Iter __end = __first + (__last - __first);
    std::fill(std::forward<_Ep>(__exec), std::move(__first), __end, __value);
    return __end;
  }

  template <class _Ep, random_access_range _Range, class _Type
#    if _LIBCPP_STD_VER >= 26
            = range_value_t<_Range>
#    endif
            , class _RawPolicy = __remove_cvref_t<_Ep>, enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires sized_range<_Range> && output_range<_Range, const _Type&>
  _LIBCPP_HIDE_FROM_ABI borrowed_iterator_t<_Range>
  operator()(_Ep&& __exec, _Range&& __range, const _Type& __value) const {
    return (*this)(std::forward<_Ep>(__exec), ranges::begin(__range), ranges::end(__range), __value);
  }
#  endif
};

inline namespace __cpo {
inline constexpr auto fill = __fill{};
} // namespace __cpo
} // namespace ranges

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_STD_VER >= 20

_LIBCPP_POP_MACROS

#endif // _LIBCPP___ALGORITHM_RANGES_FILL_H
