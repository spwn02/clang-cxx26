//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___ALGORITHM_RANGES_CONTAINS_SUBRANGE_H
#define _LIBCPP___ALGORITHM_RANGES_CONTAINS_SUBRANGE_H

#include <__algorithm/ranges_search.h>
#include <__algorithm/pstl.h>
#include <__config>
#include <__functional/identity.h>
#include <__functional/ranges_operations.h>
#include <__functional/reference_wrapper.h>
#include <__iterator/concepts.h>
#include <__iterator/indirectly_comparable.h>
#include <__iterator/projected.h>
#include <__ranges/access.h>
#include <__ranges/concepts.h>
#include <__ranges/size.h>
#include <__ranges/subrange.h>
#include <__utility/move.h>
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
struct __contains_subrange {
  template <forward_iterator _Iter1,
            sentinel_for<_Iter1> _Sent1,
            forward_iterator _Iter2,
            sentinel_for<_Iter2> _Sent2,
            class _Pred  = ranges::equal_to,
            class _Proj1 = identity,
            class _Proj2 = identity>
    requires indirectly_comparable<_Iter1, _Iter2, _Pred, _Proj1, _Proj2>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr bool static operator()(
      _Iter1 __first1,
      _Sent1 __last1,
      _Iter2 __first2,
      _Sent2 __last2,
      _Pred __pred   = {},
      _Proj1 __proj1 = {},
      _Proj2 __proj2 = {}) {
    if (__first2 == __last2)
      return true;

    auto __ret = ranges::search(
        std::move(__first1), __last1, std::move(__first2), __last2, __pred, std::ref(__proj1), std::ref(__proj2));
    return __ret.empty() == false;
  }

  template <forward_range _Range1,
            forward_range _Range2,
            class _Pred  = ranges::equal_to,
            class _Proj1 = identity,
            class _Proj2 = identity>
    requires indirectly_comparable<iterator_t<_Range1>, iterator_t<_Range2>, _Pred, _Proj1, _Proj2>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr bool static
  operator()(_Range1&& __range1, _Range2&& __range2, _Pred __pred = {}, _Proj1 __proj1 = {}, _Proj2 __proj2 = {}) {
    if constexpr (sized_range<_Range2>) {
      if (ranges::size(__range2) == 0)
        return true;
    } else {
      if (ranges::begin(__range2) == ranges::end(__range2))
        return true;
    }

    auto __ret = ranges::search(__range1, __range2, __pred, std::ref(__proj1), std::ref(__proj2));
    return __ret.empty() == false;
  }

#  if _LIBCPP_HAS_EXPERIMENTAL_PSTL
  template <class _Ep, random_access_iterator _Iter1, sized_sentinel_for<_Iter1> _Sent1,
            random_access_iterator _Iter2, sized_sentinel_for<_Iter2> _Sent2,
            class _Pred = ranges::equal_to, class _Proj1 = identity, class _Proj2 = identity,
            class _RawPolicy = __remove_cvref_t<_Ep>, enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires indirectly_comparable<_Iter1, _Iter2, _Pred, _Proj1, _Proj2>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI bool static operator()(
      _Ep&& __exec, _Iter1 __first1, _Sent1 __last1, _Iter2 __first2, _Sent2 __last2,
      _Pred __pred = {}, _Proj1 __proj1 = {}, _Proj2 __proj2 = {}) {
    if (__first2 == __last2)
      return true;
    auto __ret = ranges::search(std::forward<_Ep>(__exec), __first1, __last1, __first2, __last2,
                                std::move(__pred), std::move(__proj1), std::move(__proj2));
    return __ret.empty() == false;
  }

  template <class _Ep, random_access_range _Range1, random_access_range _Range2,
            class _Pred = ranges::equal_to, class _Proj1 = identity, class _Proj2 = identity,
            class _RawPolicy = __remove_cvref_t<_Ep>, enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires sized_range<_Range1> && sized_range<_Range2> &&
             indirectly_comparable<iterator_t<_Range1>, iterator_t<_Range2>, _Pred, _Proj1, _Proj2>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI bool static operator()(
      _Ep&& __exec, _Range1&& __range1, _Range2&& __range2,
      _Pred __pred = {}, _Proj1 __proj1 = {}, _Proj2 __proj2 = {}) {
    // A static member function has no `this` to delegate through; call the sibling static
    // overload directly by (unqualified) name instead, matching this class's own non-policy
    // overloads above (which are static for the same reason and never use (*this) either).
    return operator()(std::forward<_Ep>(__exec), ranges::begin(__range1), ranges::end(__range1),
                      ranges::begin(__range2), ranges::end(__range2), std::move(__pred), std::move(__proj1),
                      std::move(__proj2));
  }
#  endif
};

inline namespace __cpo {
inline constexpr auto contains_subrange = __contains_subrange{};
} // namespace __cpo
} // namespace ranges

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_STD_VER >= 23

_LIBCPP_POP_MACROS

#endif // _LIBCPP___ALGORITHM_RANGES_CONTAINS_SUBRANGE_H
