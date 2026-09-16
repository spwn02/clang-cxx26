//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___ALGORITHM_RANGES_COPY_IF_H
#define _LIBCPP___ALGORITHM_RANGES_COPY_IF_H

#include <__algorithm/copy_if.h>
#include <__algorithm/in_out_result.h>
#include <__algorithm/pstl.h>
#include <__config>
#include <__functional/identity.h>
#include <__functional/invoke.h>
#include <__iterator/concepts.h>
#include <__iterator/projected.h>
#include <__ranges/access.h>
#include <__ranges/concepts.h>
#include <__ranges/dangling.h>
#include <__type_traits/is_execution_policy.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/move.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

#if _LIBCPP_STD_VER >= 20

_LIBCPP_BEGIN_NAMESPACE_STD

namespace ranges {

template <class _Ip, class _Op>
using copy_if_result = in_out_result<_Ip, _Op>;

struct __copy_if {
  template <input_iterator _Iter,
            sentinel_for<_Iter> _Sent,
            weakly_incrementable _OutIter,
            class _Proj = identity,
            indirect_unary_predicate<projected<_Iter, _Proj>> _Pred>
    requires indirectly_copyable<_Iter, _OutIter>
  _LIBCPP_HIDE_FROM_ABI constexpr copy_if_result<_Iter, _OutIter>
  operator()(_Iter __first, _Sent __last, _OutIter __result, _Pred __pred, _Proj __proj = {}) const {
    auto __res = std::__copy_if(std::move(__first), std::move(__last), std::move(__result), __pred, __proj);
    return {std::move(__res.first), std::move(__res.second)};
  }

  template <input_range _Range,
            weakly_incrementable _OutIter,
            class _Proj = identity,
            indirect_unary_predicate<projected<iterator_t<_Range>, _Proj>> _Pred>
    requires indirectly_copyable<iterator_t<_Range>, _OutIter>
  _LIBCPP_HIDE_FROM_ABI constexpr copy_if_result<borrowed_iterator_t<_Range>, _OutIter>
  operator()(_Range&& __r, _OutIter __result, _Pred __pred, _Proj __proj = {}) const {
    auto __res = std::__copy_if(ranges::begin(__r), ranges::end(__r), std::move(__result), __pred, __proj);
    return {std::move(__res.first), std::move(__res.second)};
  }
#  if _LIBCPP_HAS_EXPERIMENTAL_PSTL
  template <class _Ep,
            random_access_iterator _Iter,
            sized_sentinel_for<_Iter> _Sent,
            class _OutIter,
            class _Pred,
            class _Proj                                          = identity,
            class _RawPolicy                                     = __remove_cvref_t<_Ep>,
            enable_if_t<is_execution_policy_v<_RawPolicy>, int>   = 0>
    requires indirectly_copyable<_Iter, _OutIter>
  _LIBCPP_HIDE_FROM_ABI copy_if_result<_Iter, _OutIter>
  operator()(_Ep&& __exec, _Iter __first, _Sent __last, _OutIter __result, _Pred __pred, _Proj __proj = {}) const {
    _Iter __end = __first + (__last - __first);
    auto __res  = std::copy_if(
        std::forward<_Ep>(__exec), __first, __end, std::move(__result),
        [&__pred, &__proj](auto&& __elem) { return std::invoke(__pred, std::invoke(__proj, __elem)); });
    return {std::move(__end), std::move(__res)};
  }

  template <class _Ep,
            random_access_range _Range,
            class _OutIter,
            class _Pred,
            class _Proj                                        = identity,
            class _RawPolicy                                   = __remove_cvref_t<_Ep>,
            enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires sized_range<_Range> && indirectly_copyable<iterator_t<_Range>, _OutIter>
  _LIBCPP_HIDE_FROM_ABI copy_if_result<borrowed_iterator_t<_Range>, _OutIter>
  operator()(_Ep&& __exec, _Range&& __r, _OutIter __result, _Pred __pred, _Proj __proj = {}) const {
    return (*this)(
        std::forward<_Ep>(__exec), ranges::begin(__r), ranges::end(__r), std::move(__result), std::move(__pred),
        std::move(__proj));
  }
#  endif
};

inline namespace __cpo {
inline constexpr auto copy_if = __copy_if{};
} // namespace __cpo
} // namespace ranges

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_STD_VER >= 20

_LIBCPP_POP_MACROS

#endif // _LIBCPP___ALGORITHM_RANGES_COPY_IF_H
