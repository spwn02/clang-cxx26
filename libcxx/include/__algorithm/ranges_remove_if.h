//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___ALGORITHM_RANGES_REMOVE_IF_H
#define _LIBCPP___ALGORITHM_RANGES_REMOVE_IF_H
#include <__config>

#include <__algorithm/pstl.h>
#include <__algorithm/ranges_find_if.h>
#include <__functional/identity.h>
#include <__functional/invoke.h>
#include <__functional/ranges_operations.h>
#include <__iterator/concepts.h>
#include <__iterator/iter_move.h>
#include <__iterator/permutable.h>
#include <__iterator/projected.h>
#include <__ranges/access.h>
#include <__ranges/concepts.h>
#include <__ranges/subrange.h>
#include <__utility/move.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

#if _LIBCPP_STD_VER >= 20

_LIBCPP_BEGIN_NAMESPACE_STD

namespace ranges {

template <class _Iter, class _Sent, class _Proj, class _Pred>
_LIBCPP_HIDE_FROM_ABI constexpr subrange<_Iter>
__remove_if_impl(_Iter __first, _Sent __last, _Pred& __pred, _Proj& __proj) {
  auto __new_end = ranges::__find_if_impl(__first, __last, __pred, __proj);
  if (__new_end == __last)
    return {__new_end, __new_end};

  _Iter __i = __new_end;
  while (++__i != __last) {
    if (!std::invoke(__pred, std::invoke(__proj, *__i))) {
      *__new_end = ranges::iter_move(__i);
      ++__new_end;
    }
  }
  return {__new_end, __i};
}

struct __remove_if {
  template <permutable _Iter,
            sentinel_for<_Iter> _Sent,
            class _Proj = identity,
            indirect_unary_predicate<projected<_Iter, _Proj>> _Pred>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr subrange<_Iter>
  operator()(_Iter __first, _Sent __last, _Pred __pred, _Proj __proj = {}) const {
    return ranges::__remove_if_impl(std::move(__first), std::move(__last), __pred, __proj);
  }

  template <forward_range _Range,
            class _Proj = identity,
            indirect_unary_predicate<projected<iterator_t<_Range>, _Proj>> _Pred>
    requires permutable<iterator_t<_Range>>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr borrowed_subrange_t<_Range>
  operator()(_Range&& __range, _Pred __pred, _Proj __proj = {}) const {
    return ranges::__remove_if_impl(ranges::begin(__range), ranges::end(__range), __pred, __proj);
  }
#  if _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_EXPERIMENTAL_PSTL && !defined(_LIBCPP_FREESTANDING)
  template <class _Ep,
            random_access_iterator _Iter,
            sized_sentinel_for<_Iter> _Sent,
            class _Proj = identity,
            indirect_unary_predicate<projected<_Iter, _Proj>> _Pred,
            class _RawPolicy                                    = __remove_cvref_t<_Ep>,
            enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires permutable<_Iter>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI subrange<_Iter>
  operator()(_Ep&& __exec, _Iter __first, _Sent __last, _Pred __pred, _Proj __proj = {}) const {
    _Iter __end    = __first + (__last - __first);
    auto __new_end = std::remove_if(std::forward<_Ep>(__exec), __first, __end, [&__pred, &__proj](auto&& __elem) {
      return std::invoke(__pred, std::invoke(__proj, __elem));
    });
    return {std::move(__new_end), std::move(__end)};
  }

  template <class _Ep,
            random_access_range _Range,
            class _Proj = identity,
            indirect_unary_predicate<projected<iterator_t<_Range>, _Proj>> _Pred,
            class _RawPolicy                                    = __remove_cvref_t<_Ep>,
            enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires sized_range<_Range> && permutable<iterator_t<_Range>>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI borrowed_subrange_t<_Range>
  operator()(_Ep&& __exec, _Range&& __range, _Pred __pred, _Proj __proj = {}) const {
    return (*this)(
        std::forward<_Ep>(__exec),
        ranges::begin(__range),
        ranges::begin(__range) + ranges::size(__range),
        std::move(__pred),
        std::move(__proj));
  }
#  endif
};

inline namespace __cpo {
inline constexpr auto remove_if = __remove_if{};
} // namespace __cpo
} // namespace ranges

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_STD_VER >= 20

_LIBCPP_POP_MACROS

#endif // _LIBCPP___ALGORITHM_RANGES_REMOVE_IF_H
