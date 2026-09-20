//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___ALGORITHM_RANGES_SEARCH_H
#define _LIBCPP___ALGORITHM_RANGES_SEARCH_H

#include <__algorithm/iterator_operations.h>
#include <__algorithm/pstl.h>
#include <__algorithm/search.h>
#include <__config>
#include <__functional/identity.h>
#include <__functional/invoke.h>
#include <__functional/ranges_operations.h>
#include <__iterator/advance.h>
#include <__iterator/concepts.h>
#include <__iterator/distance.h>
#include <__iterator/indirectly_comparable.h>
#include <__ranges/access.h>
#include <__ranges/concepts.h>
#include <__ranges/size.h>
#include <__ranges/subrange.h>
#include <__utility/pair.h>
#include <__utility/forward.h>
#include <__type_traits/is_execution_policy.h>
#include <__type_traits/remove_cvref.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_STD_VER >= 20

_LIBCPP_BEGIN_NAMESPACE_STD

namespace ranges {
struct __search {
  template <class _Iter1, class _Sent1, class _Iter2, class _Sent2, class _Pred, class _Proj1, class _Proj2>
  _LIBCPP_HIDE_FROM_ABI static constexpr subrange<_Iter1> __ranges_search_impl(
      _Iter1 __first1,
      _Sent1 __last1,
      _Iter2 __first2,
      _Sent2 __last2,
      _Pred& __pred,
      _Proj1& __proj1,
      _Proj2& __proj2) {
    if constexpr (sized_sentinel_for<_Sent2, _Iter2>) {
      auto __size2 = ranges::distance(__first2, __last2);
      if (__size2 == 0)
        return {__first1, __first1};

      if constexpr (sized_sentinel_for<_Sent1, _Iter1>) {
        auto __size1 = ranges::distance(__first1, __last1);
        if (__size1 < __size2) {
          ranges::advance(__first1, __last1);
          return {__first1, __first1};
        }

        if constexpr (random_access_iterator<_Iter1> && random_access_iterator<_Iter2>) {
          auto __ret = std::__search_random_access_impl<_RangeAlgPolicy>(
              __first1, __last1, __first2, __last2, __pred, __proj1, __proj2, __size1, __size2);
          return {__ret.first, __ret.second};
        }
      }
    }

    auto __ret =
        std::__search_forward_impl<_RangeAlgPolicy>(__first1, __last1, __first2, __last2, __pred, __proj1, __proj2);
    return {__ret.first, __ret.second};
  }

  template <forward_iterator _Iter1,
            sentinel_for<_Iter1> _Sent1,
            forward_iterator _Iter2,
            sentinel_for<_Iter2> _Sent2,
            class _Pred  = ranges::equal_to,
            class _Proj1 = identity,
            class _Proj2 = identity>
    requires indirectly_comparable<_Iter1, _Iter2, _Pred, _Proj1, _Proj2>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr subrange<_Iter1> operator()(
      _Iter1 __first1,
      _Sent1 __last1,
      _Iter2 __first2,
      _Sent2 __last2,
      _Pred __pred   = {},
      _Proj1 __proj1 = {},
      _Proj2 __proj2 = {}) const {
    return __ranges_search_impl(__first1, __last1, __first2, __last2, __pred, __proj1, __proj2);
  }

  template <forward_range _Range1,
            forward_range _Range2,
            class _Pred  = ranges::equal_to,
            class _Proj1 = identity,
            class _Proj2 = identity>
    requires indirectly_comparable<iterator_t<_Range1>, iterator_t<_Range2>, _Pred, _Proj1, _Proj2>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr borrowed_subrange_t<_Range1> operator()(
      _Range1&& __range1, _Range2&& __range2, _Pred __pred = {}, _Proj1 __proj1 = {}, _Proj2 __proj2 = {}) const {
    auto __first1 = ranges::begin(__range1);
    if constexpr (sized_range<_Range2>) {
      auto __size2 = ranges::size(__range2);
      if (__size2 == 0)
        return {__first1, __first1};
      if constexpr (sized_range<_Range1>) {
        auto __size1 = ranges::size(__range1);
        if (__size1 < __size2) {
          ranges::advance(__first1, ranges::end(__range1));
          return {__first1, __first1};
        }
      }
    }

    return __ranges_search_impl(
        ranges::begin(__range1),
        ranges::end(__range1),
        ranges::begin(__range2),
        ranges::end(__range2),
        __pred,
        __proj1,
        __proj2);
  }

#  if _LIBCPP_HAS_EXPERIMENTAL_PSTL
  template <class _Ep, random_access_iterator _Iter1, sized_sentinel_for<_Iter1> _Sent1,
            random_access_iterator _Iter2, sized_sentinel_for<_Iter2> _Sent2,
            class _Pred = ranges::equal_to, class _Proj1 = identity, class _Proj2 = identity,
            class _RawPolicy = __remove_cvref_t<_Ep>, enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires indirectly_comparable<_Iter1, _Iter2, _Pred, _Proj1, _Proj2>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI subrange<_Iter1> operator()(
      _Ep&& __exec, _Iter1 __first1, _Sent1 __last1, _Iter2 __first2, _Sent2 __last2,
      _Pred __pred = {}, _Proj1 __proj1 = {}, _Proj2 __proj2 = {}) const {
    _Iter1 __end1 = __first1 + (__last1 - __first1);
    _Iter2 __end2 = __first2 + (__last2 - __first2);
    _Iter1 __result = std::search(
        std::forward<_Ep>(__exec), __first1, __end1, __first2, __end2,
        [__pred = static_cast<_Pred&&>(__pred), __proj1 = static_cast<_Proj1&&>(__proj1),
         __proj2 = static_cast<_Proj2&&>(__proj2)](
            auto&& __a, auto&& __b) mutable {
          return std::invoke(__pred, std::invoke(__proj1, std::forward<decltype(__a)>(__a)),
                             std::invoke(__proj2, std::forward<decltype(__b)>(__b)));
        });
    if (__result == __end1)
      return {__end1, __end1};
    return {__result, __result + (__last2 - __first2)};
  }

  template <class _Ep, random_access_range _Range1, random_access_range _Range2,
            class _Pred = ranges::equal_to, class _Proj1 = identity, class _Proj2 = identity,
            class _RawPolicy = __remove_cvref_t<_Ep>, enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires sized_range<_Range1> && sized_range<_Range2> &&
             indirectly_comparable<iterator_t<_Range1>, iterator_t<_Range2>, _Pred, _Proj1, _Proj2>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI borrowed_subrange_t<_Range1> operator()(
      _Ep&& __exec, _Range1&& __range1, _Range2&& __range2,
      _Pred __pred = {}, _Proj1 __proj1 = {}, _Proj2 __proj2 = {}) const {
    return (*this)(std::forward<_Ep>(__exec), ranges::begin(__range1), ranges::end(__range1),
                   ranges::begin(__range2), ranges::end(__range2), static_cast<_Pred&&>(__pred),
                   static_cast<_Proj1&&>(__proj1), static_cast<_Proj2&&>(__proj2));
  }
#  endif
};

inline namespace __cpo {
inline constexpr auto search = __search{};
} // namespace __cpo
} // namespace ranges

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_STD_VER >= 20

#endif // _LIBCPP___ALGORITHM_RANGES_SEARCH_H
