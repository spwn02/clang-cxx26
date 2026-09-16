//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___ALGORITHM_RANGES_STABLE_PARTITION_H
#define _LIBCPP___ALGORITHM_RANGES_STABLE_PARTITION_H

#include <__algorithm/iterator_operations.h>
#include <__algorithm/make_projected.h>
#include <__algorithm/pstl.h>
#include <__algorithm/ranges_iterator_concept.h>
#include <__algorithm/stable_partition.h>
#include <__config>
#include <__functional/identity.h>
#include <__functional/invoke.h>
#include <__functional/ranges_operations.h>
#include <__iterator/concepts.h>
#include <__iterator/iterator_traits.h>
#include <__iterator/next.h>
#include <__iterator/permutable.h>
#include <__iterator/projected.h>
#include <__ranges/access.h>
#include <__ranges/concepts.h>
#include <__ranges/dangling.h>
#include <__ranges/subrange.h>
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
struct __stable_partition {
  template <class _Iter, class _Sent, class _Proj, class _Pred>
  _LIBCPP_HIDE_FROM_ABI static _LIBCPP_CONSTEXPR_SINCE_CXX26 subrange<__remove_cvref_t<_Iter>>
  __stable_partition_fn_impl(_Iter&& __first, _Sent&& __last, _Pred&& __pred, _Proj&& __proj) {
    auto __last_iter = ranges::next(__first, __last);

    auto&& __projected_pred = std::__make_projected(__pred, __proj);
    auto __result           = std::__stable_partition<_RangeAlgPolicy>(
        std::move(__first), __last_iter, __projected_pred, __iterator_concept<_Iter>());

    return {std::move(__result), std::move(__last_iter)};
  }

  template <bidirectional_iterator _Iter,
            sentinel_for<_Iter> _Sent,
            class _Proj = identity,
            indirect_unary_predicate<projected<_Iter, _Proj>> _Pred>
    requires permutable<_Iter>
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 subrange<_Iter>
  operator()(_Iter __first, _Sent __last, _Pred __pred, _Proj __proj = {}) const {
    return __stable_partition_fn_impl(__first, __last, __pred, __proj);
  }

  template <bidirectional_range _Range,
            class _Proj = identity,
            indirect_unary_predicate<projected<iterator_t<_Range>, _Proj>> _Pred>
    requires permutable<iterator_t<_Range>>
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 borrowed_subrange_t<_Range>
  operator()(_Range&& __range, _Pred __pred, _Proj __proj = {}) const {
    return __stable_partition_fn_impl(ranges::begin(__range), ranges::end(__range), __pred, __proj);
  }
#  if _LIBCPP_HAS_EXPERIMENTAL_PSTL
  template <class _Ep,
            random_access_iterator _Iter,
            sized_sentinel_for<_Iter> _Sent,
            class _Pred,
            class _Proj                                        = identity,
            class _RawPolicy                                   = __remove_cvref_t<_Ep>,
            enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires permutable<_Iter>
  _LIBCPP_HIDE_FROM_ABI subrange<_Iter>
  operator()(_Ep&& __exec, _Iter __first, _Sent __last, _Pred __pred, _Proj __proj = {}) const {
    _Iter __end = __first + (__last - __first);
    auto __mid  = std::stable_partition(
        std::forward<_Ep>(__exec), __first, __end,
        [&__pred, &__proj](auto&& __elem) { return std::invoke(__pred, std::invoke(__proj, __elem)); });
    return {std::move(__mid), std::move(__end)};
  }

  template <class _Ep,
            random_access_range _Range,
            class _Pred,
            class _Proj                                        = identity,
            class _RawPolicy                                   = __remove_cvref_t<_Ep>,
            enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires sized_range<_Range> && permutable<iterator_t<_Range>>
  _LIBCPP_HIDE_FROM_ABI borrowed_subrange_t<_Range>
  operator()(_Ep&& __exec, _Range&& __r, _Pred __pred, _Proj __proj = {}) const {
    return (*this)(std::forward<_Ep>(__exec), ranges::begin(__r), ranges::end(__r), std::move(__pred), std::move(__proj));
  }
#  endif
};

inline namespace __cpo {
inline constexpr auto stable_partition = __stable_partition{};
} // namespace __cpo
} // namespace ranges

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_STD_VER >= 20

_LIBCPP_POP_MACROS

#endif // _LIBCPP___ALGORITHM_RANGES_STABLE_PARTITION_H
