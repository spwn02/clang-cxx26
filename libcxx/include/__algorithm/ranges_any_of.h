//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___ALGORITHM_RANGES_ANY_OF_H
#define _LIBCPP___ALGORITHM_RANGES_ANY_OF_H

#include <__algorithm/any_of.h>
#include <__algorithm/pstl.h>
#include <__config>
#include <__functional/identity.h>
#include <__functional/invoke.h>
#include <__iterator/concepts.h>
#include <__iterator/projected.h>
#include <__ranges/access.h>
#include <__ranges/concepts.h>
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
struct __any_of {
  template <input_iterator _Iter,
            sentinel_for<_Iter> _Sent,
            class _Proj = identity,
            indirect_unary_predicate<projected<_Iter, _Proj>> _Pred>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr bool
  operator()(_Iter __first, _Sent __last, _Pred __pred = {}, _Proj __proj = {}) const {
    return std::__any_of(std::move(__first), std::move(__last), __pred, __proj);
  }

  template <input_range _Range,
            class _Proj = identity,
            indirect_unary_predicate<projected<iterator_t<_Range>, _Proj>> _Pred>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr bool
  operator()(_Range&& __range, _Pred __pred, _Proj __proj = {}) const {
    return std::__any_of(ranges::begin(__range), ranges::end(__range), __pred, __proj);
  }

#  if _LIBCPP_HAS_EXPERIMENTAL_PSTL
  template <class _Ep,
            random_access_iterator _Iter,
            sized_sentinel_for<_Iter> _Sent,
            class _Proj = identity,
            indirect_unary_predicate<projected<_Iter, _Proj>> _Pred,
            class _RawPolicy                                    = __remove_cvref_t<_Ep>,
            enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI bool
  operator()(_Ep&& __exec, _Iter __first, _Sent __last, _Pred __pred, _Proj __proj = {}) const {
    _Iter __end = __first + (__last - __first);
    return std::any_of(
        std::forward<_Ep>(__exec),
        std::move(__first),
        std::move(__end),
        [__pred = std::move(__pred), __proj = std::move(__proj)](auto&& __value) mutable {
          return std::invoke(__pred, std::invoke(__proj, std::forward<decltype(__value)>(__value)));
        });
  }

  template <class _Ep,
            random_access_range _Range,
            class _Proj = identity,
            indirect_unary_predicate<projected<iterator_t<_Range>, _Proj>> _Pred,
            class _RawPolicy                                    = __remove_cvref_t<_Ep>,
            enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires sized_range<_Range>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI bool
  operator()(_Ep&& __exec, _Range&& __range, _Pred __pred, _Proj __proj = {}) const {
    return (*this)(
        std::forward<_Ep>(__exec), ranges::begin(__range), ranges::end(__range), std::move(__pred), std::move(__proj));
  }
#  endif
};

inline namespace __cpo {
inline constexpr auto any_of = __any_of{};
} // namespace __cpo
} // namespace ranges

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_STD_VER >= 20

_LIBCPP_POP_MACROS

#endif // _LIBCPP___ALGORITHM_RANGES_ANY_OF_H
