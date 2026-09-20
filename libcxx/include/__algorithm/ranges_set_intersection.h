//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___ALGORITHM_RANGES_SET_INTERSECTION_H
#define _LIBCPP___ALGORITHM_RANGES_SET_INTERSECTION_H

#include <__algorithm/in_in_out_result.h>
#include <__algorithm/pstl.h>
#include <__algorithm/iterator_operations.h>
#include <__algorithm/make_projected.h>
#include <__algorithm/set_intersection.h>
#include <__config>
#include <__functional/identity.h>
#include <__functional/invoke.h>
#include <__functional/ranges_operations.h>
#include <__iterator/concepts.h>
#include <__iterator/mergeable.h>
#include <__ranges/access.h>
#include <__ranges/concepts.h>
#include <__ranges/dangling.h>
#include <__utility/move.h>
#include <__utility/forward.h>
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

template <class _InIter1, class _InIter2, class _OutIter>
using set_intersection_result = in_in_out_result<_InIter1, _InIter2, _OutIter>;

struct __set_intersection {
  template <input_iterator _InIter1,
            sentinel_for<_InIter1> _Sent1,
            input_iterator _InIter2,
            sentinel_for<_InIter2> _Sent2,
            weakly_incrementable _OutIter,
            class _Comp  = less,
            class _Proj1 = identity,
            class _Proj2 = identity>
    requires mergeable<_InIter1, _InIter2, _OutIter, _Comp, _Proj1, _Proj2>
  _LIBCPP_HIDE_FROM_ABI constexpr set_intersection_result<_InIter1, _InIter2, _OutIter> operator()(
      _InIter1 __first1,
      _Sent1 __last1,
      _InIter2 __first2,
      _Sent2 __last2,
      _OutIter __result,
      _Comp __comp   = {},
      _Proj1 __proj1 = {},
      _Proj2 __proj2 = {}) const {
    auto __ret = std::__set_intersection<_RangeAlgPolicy>(
        std::move(__first1),
        std::move(__last1),
        std::move(__first2),
        std::move(__last2),
        std::move(__result),
        ranges::__make_projected_comp(__comp, __proj1, __proj2));
    return {std::move(__ret.__in1_), std::move(__ret.__in2_), std::move(__ret.__out_)};
  }

  template <input_range _Range1,
            input_range _Range2,
            weakly_incrementable _OutIter,
            class _Comp  = less,
            class _Proj1 = identity,
            class _Proj2 = identity>
    requires mergeable<iterator_t<_Range1>, iterator_t<_Range2>, _OutIter, _Comp, _Proj1, _Proj2>
  _LIBCPP_HIDE_FROM_ABI constexpr set_intersection_result<borrowed_iterator_t<_Range1>,
                                                          borrowed_iterator_t<_Range2>,
                                                          _OutIter>
  operator()(_Range1&& __range1,
             _Range2&& __range2,
             _OutIter __result,
             _Comp __comp   = {},
             _Proj1 __proj1 = {},
             _Proj2 __proj2 = {}) const {
    auto __ret = std::__set_intersection<_RangeAlgPolicy>(
        ranges::begin(__range1),
        ranges::end(__range1),
        ranges::begin(__range2),
        ranges::end(__range2),
        std::move(__result),
        ranges::__make_projected_comp(__comp, __proj1, __proj2));
    return {std::move(__ret.__in1_), std::move(__ret.__in2_), std::move(__ret.__out_)};
  }

#  if _LIBCPP_HAS_EXPERIMENTAL_PSTL
  template <class _Ep, random_access_iterator _InIter1, sized_sentinel_for<_InIter1> _Sent1,
            random_access_iterator _InIter2, sized_sentinel_for<_InIter2> _Sent2, weakly_incrementable _OutIter,
            class _Comp = less, class _Proj1 = identity, class _Proj2 = identity,
            class _RawPolicy = __remove_cvref_t<_Ep>, enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires mergeable<_InIter1, _InIter2, _OutIter, _Comp, _Proj1, _Proj2>
  _LIBCPP_HIDE_FROM_ABI set_intersection_result<_InIter1, _InIter2, _OutIter> operator()(
      _Ep&& __exec, _InIter1 __first1, _Sent1 __last1, _InIter2 __first2, _Sent2 __last2, _OutIter __result,
      _Comp __comp = {}, _Proj1 __proj1 = {}, _Proj2 __proj2 = {}) const {
    _InIter1 __end1 = __first1 + (__last1 - __first1);
    _InIter2 __end2 = __first2 + (__last2 - __first2);
    _OutIter __destination = std::set_intersection(
        std::forward<_Ep>(__exec), std::move(__first1), __end1, std::move(__first2), __end2, std::move(__result),
        [__comp = std::move(__comp), __proj1 = std::move(__proj1), __proj2 = std::move(__proj2)](auto&& __a, auto&& __b) mutable {
          return std::invoke(__comp, std::invoke(__proj1, std::forward<decltype(__a)>(__a)),
                             std::invoke(__proj2, std::forward<decltype(__b)>(__b)));
        });
    return {std::move(__end1), std::move(__end2), std::move(__destination)};
  }

  template <class _Ep, random_access_range _Range1, random_access_range _Range2, weakly_incrementable _OutIter,
            class _Comp = less, class _Proj1 = identity, class _Proj2 = identity,
            class _RawPolicy = __remove_cvref_t<_Ep>, enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires sized_range<_Range1> && sized_range<_Range2> &&
             mergeable<iterator_t<_Range1>, iterator_t<_Range2>, _OutIter, _Comp, _Proj1, _Proj2>
  _LIBCPP_HIDE_FROM_ABI set_intersection_result<borrowed_iterator_t<_Range1>, borrowed_iterator_t<_Range2>, _OutIter>
  operator()(_Ep&& __exec, _Range1&& __range1, _Range2&& __range2, _OutIter __result, _Comp __comp = {},
             _Proj1 __proj1 = {}, _Proj2 __proj2 = {}) const {
    return (*this)(std::forward<_Ep>(__exec), ranges::begin(__range1), ranges::end(__range1), ranges::begin(__range2),
                   ranges::end(__range2), std::move(__result), std::move(__comp), std::move(__proj1), std::move(__proj2));
  }
#  endif
};

inline namespace __cpo {
inline constexpr auto set_intersection = __set_intersection{};
} // namespace __cpo
} // namespace ranges

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_STD_VER >= 20

_LIBCPP_POP_MACROS

#endif // _LIBCPP___ALGORITHM_RANGES_SET_INTERSECTION_H
