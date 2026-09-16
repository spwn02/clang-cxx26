//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___ALGORITHM_RANGES_INPLACE_MERGE_H
#define _LIBCPP___ALGORITHM_RANGES_INPLACE_MERGE_H

#include <__algorithm/inplace_merge.h>
#include <__algorithm/pstl.h>
#include <__algorithm/iterator_operations.h>
#include <__algorithm/make_projected.h>
#include <__config>
#include <__functional/identity.h>
#include <__functional/invoke.h>
#include <__functional/ranges_operations.h>
#include <__iterator/concepts.h>
#include <__iterator/iterator_traits.h>
#include <__iterator/next.h>
#include <__iterator/projected.h>
#include <__iterator/sortable.h>
#include <__ranges/access.h>
#include <__ranges/concepts.h>
#include <__ranges/dangling.h>
#include <__utility/forward.h>
#include <__utility/move.h>
#include <__type_traits/is_execution_policy.h>
#include <__type_traits/remove_cvref.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

#if _LIBCPP_STD_VER >= 20

_LIBCPP_BEGIN_NAMESPACE_STD

namespace ranges {
struct __inplace_merge {
  template <class _Iter, class _Sent, class _Comp, class _Proj>
  _LIBCPP_HIDE_FROM_ABI static _LIBCPP_CONSTEXPR_SINCE_CXX26 auto
  __inplace_merge_impl(_Iter __first, _Iter __middle, _Sent __last, _Comp&& __comp, _Proj&& __proj) {
    auto __last_iter = ranges::next(__middle, __last);
    std::__inplace_merge<_RangeAlgPolicy>(
        std::move(__first), std::move(__middle), __last_iter, std::__make_projected(__comp, __proj));
    return __last_iter;
  }

  template <bidirectional_iterator _Iter, sentinel_for<_Iter> _Sent, class _Comp = ranges::less, class _Proj = identity>
    requires sortable<_Iter, _Comp, _Proj>
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Iter
  operator()(_Iter __first, _Iter __middle, _Sent __last, _Comp __comp = {}, _Proj __proj = {}) const {
    return __inplace_merge_impl(
        std::move(__first), std::move(__middle), std::move(__last), std::move(__comp), std::move(__proj));
  }

  template <bidirectional_range _Range, class _Comp = ranges::less, class _Proj = identity>
    requires sortable<iterator_t<_Range>, _Comp, _Proj>
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 borrowed_iterator_t<_Range>
  operator()(_Range&& __range, iterator_t<_Range> __middle, _Comp __comp = {}, _Proj __proj = {}) const {
    return __inplace_merge_impl(
        ranges::begin(__range), std::move(__middle), ranges::end(__range), std::move(__comp), std::move(__proj));
  }

#  if _LIBCPP_HAS_EXPERIMENTAL_PSTL
  template <class _Ep, random_access_iterator _Iter, sized_sentinel_for<_Iter> _Sent,
            class _Comp = ranges::less, class _Proj = identity,
            class _RawPolicy = __remove_cvref_t<_Ep>, enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires sortable<_Iter, _Comp, _Proj>
  _LIBCPP_HIDE_FROM_ABI _Iter operator()(
      _Ep&& __exec, _Iter __first, _Iter __middle, _Sent __last, _Comp __comp = {}, _Proj __proj = {}) const {
    _Iter __end = __first + (__last - __first);
    std::inplace_merge(std::forward<_Ep>(__exec), __first, __middle, __end,
                       [&__comp, &__proj](auto&& __a, auto&& __b) {
                         return std::invoke(__comp, std::invoke(__proj, std::forward<decltype(__a)>(__a)),
                                            std::invoke(__proj, std::forward<decltype(__b)>(__b)));
                       });
    return __end;
  }

  template <class _Ep, random_access_range _Range, class _Comp = ranges::less, class _Proj = identity,
            class _RawPolicy = __remove_cvref_t<_Ep>, enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires sized_range<_Range> && sortable<iterator_t<_Range>, _Comp, _Proj>
  _LIBCPP_HIDE_FROM_ABI borrowed_iterator_t<_Range> operator()(
      _Ep&& __exec, _Range&& __range, iterator_t<_Range> __middle,
      _Comp __comp = {}, _Proj __proj = {}) const {
    return (*this)(std::forward<_Ep>(__exec), ranges::begin(__range), std::move(__middle), ranges::end(__range),
                   std::move(__comp), std::move(__proj));
  }
#  endif
};

inline namespace __cpo {
inline constexpr auto inplace_merge = __inplace_merge{};
} // namespace __cpo
} // namespace ranges

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_STD_VER >= 20

_LIBCPP_POP_MACROS

#endif // _LIBCPP___ALGORITHM_RANGES_INPLACE_MERGE_H
