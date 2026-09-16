//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___ALGORITHM_RANGES_STARTS_WITH_H
#define _LIBCPP___ALGORITHM_RANGES_STARTS_WITH_H

#include <__algorithm/in_in_result.h>
#include <__algorithm/ranges_mismatch.h>
#include <__algorithm/pstl.h>
#include <__config>
#include <__functional/identity.h>
#include <__functional/invoke.h>
#include <__functional/ranges_operations.h>
#include <__iterator/concepts.h>
#include <__iterator/indirectly_comparable.h>
#include <__ranges/access.h>
#include <__ranges/concepts.h>
#include <__utility/move.h>
#include <__utility/forward.h>
#include <__type_traits/is_execution_policy.h>
#include <__type_traits/remove_cvref.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

#if _LIBCPP_STD_VER >= 23

_LIBCPP_BEGIN_NAMESPACE_STD

namespace ranges {
struct __starts_with {
  template <input_iterator _Iter1,
            sentinel_for<_Iter1> _Sent1,
            input_iterator _Iter2,
            sentinel_for<_Iter2> _Sent2,
            class _Pred  = ranges::equal_to,
            class _Proj1 = identity,
            class _Proj2 = identity>
    requires indirectly_comparable<_Iter1, _Iter2, _Pred, _Proj1, _Proj2>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI static constexpr bool operator()(
      _Iter1 __first1,
      _Sent1 __last1,
      _Iter2 __first2,
      _Sent2 __last2,
      _Pred __pred   = {},
      _Proj1 __proj1 = {},
      _Proj2 __proj2 = {}) {
    return __mismatch::__go(
               std::move(__first1),
               std::move(__last1),
               std::move(__first2),
               std::move(__last2),
               __pred,
               __proj1,
               __proj2)
               .in2 == __last2;
  }

  template <input_range _Range1,
            input_range _Range2,
            class _Pred  = ranges::equal_to,
            class _Proj1 = identity,
            class _Proj2 = identity>
    requires indirectly_comparable<iterator_t<_Range1>, iterator_t<_Range2>, _Pred, _Proj1, _Proj2>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI static constexpr bool
  operator()(_Range1&& __range1, _Range2&& __range2, _Pred __pred = {}, _Proj1 __proj1 = {}, _Proj2 __proj2 = {}) {
    return __mismatch::__go(
               ranges::begin(__range1),
               ranges::end(__range1),
               ranges::begin(__range2),
               ranges::end(__range2),
               __pred,
               __proj1,
               __proj2)
               .in2 == ranges::end(__range2);
  }

#  if _LIBCPP_HAS_EXPERIMENTAL_PSTL
  template <class _Ep, random_access_iterator _Iter1, sized_sentinel_for<_Iter1> _Sent1,
            random_access_iterator _Iter2, sized_sentinel_for<_Iter2> _Sent2,
            class _Pred = ranges::equal_to, class _Proj1 = identity, class _Proj2 = identity,
            class _RawPolicy = __remove_cvref_t<_Ep>, enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires indirectly_comparable<_Iter1, _Iter2, _Pred, _Proj1, _Proj2>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI bool operator()(
      _Ep&& __exec, _Iter1 __first1, _Sent1 __last1, _Iter2 __first2, _Sent2 __last2,
      _Pred __pred = {}, _Proj1 __proj1 = {}, _Proj2 __proj2 = {}) const {
    _Iter1 __end1 = __first1 + (__last1 - __first1);
    _Iter2 __end2 = __first2 + (__last2 - __first2);
    return ranges::mismatch(std::forward<_Ep>(__exec), std::move(__first1), std::move(__end1),
                            std::move(__first2), std::move(__end2), std::move(__pred), std::move(__proj1),
                            std::move(__proj2)).in2 == __end2;
  }

  template <class _Ep, random_access_range _Range1, random_access_range _Range2,
            class _Pred = ranges::equal_to, class _Proj1 = identity, class _Proj2 = identity,
            class _RawPolicy = __remove_cvref_t<_Ep>, enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires sized_range<_Range1> && sized_range<_Range2> &&
             indirectly_comparable<iterator_t<_Range1>, iterator_t<_Range2>, _Pred, _Proj1, _Proj2>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI bool operator()(
      _Ep&& __exec, _Range1&& __range1, _Range2&& __range2,
      _Pred __pred = {}, _Proj1 __proj1 = {}, _Proj2 __proj2 = {}) const {
    return (*this)(std::forward<_Ep>(__exec), ranges::begin(__range1), ranges::end(__range1),
                   ranges::begin(__range2), ranges::end(__range2), std::move(__pred), std::move(__proj1), std::move(__proj2));
  }
#  endif
};
inline namespace __cpo {
inline constexpr auto starts_with = __starts_with{};
} // namespace __cpo
} // namespace ranges

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_STD_VER >= 23

_LIBCPP_POP_MACROS

#endif // _LIBCPP___ALGORITHM_RANGES_STARTS_WITH_H
