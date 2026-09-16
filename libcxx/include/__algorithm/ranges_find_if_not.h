//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___ALGORITHM_RANGES_FIND_IF_NOT_H
#define _LIBCPP___ALGORITHM_RANGES_FIND_IF_NOT_H

#include <__algorithm/ranges_find_if.h>
#include <__algorithm/pstl.h>
#include <__config>
#include <__functional/identity.h>
#include <__functional/invoke.h>
#include <__functional/ranges_operations.h>
#include <__iterator/concepts.h>
#include <__iterator/projected.h>
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
struct __find_if_not {
  template <input_iterator _Ip,
            sentinel_for<_Ip> _Sp,
            class _Proj = identity,
            indirect_unary_predicate<projected<_Ip, _Proj>> _Pred>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr _Ip
  operator()(_Ip __first, _Sp __last, _Pred __pred, _Proj __proj = {}) const {
    auto __pred2 = [&](auto&& __e) -> bool { return !std::invoke(__pred, std::forward<decltype(__e)>(__e)); };
    return ranges::__find_if_impl(std::move(__first), std::move(__last), __pred2, __proj);
  }

  template <input_range _Rp, class _Proj = identity, indirect_unary_predicate<projected<iterator_t<_Rp>, _Proj>> _Pred>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr borrowed_iterator_t<_Rp>
  operator()(_Rp&& __r, _Pred __pred, _Proj __proj = {}) const {
    auto __pred2 = [&](auto&& __e) -> bool { return !std::invoke(__pred, std::forward<decltype(__e)>(__e)); };
    return ranges::__find_if_impl(ranges::begin(__r), ranges::end(__r), __pred2, __proj);
  }

#  if _LIBCPP_HAS_EXPERIMENTAL_PSTL
  template <class _Ep, random_access_iterator _Ip, sized_sentinel_for<_Ip> _Sp,
            class _Proj = identity, indirect_unary_predicate<projected<_Ip, _Proj>> _Pred,
            class _RawPolicy = __remove_cvref_t<_Ep>, enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI _Ip
  operator()(_Ep&& __exec, _Ip __first, _Sp __last, _Pred __pred, _Proj __proj = {}) const {
    _Ip __end = __first + (__last - __first);
    return std::find_if_not(std::forward<_Ep>(__exec), std::move(__first), std::move(__end),
                            [__pred = std::move(__pred), __proj = std::move(__proj)](auto&& __value) mutable {
                              return std::invoke(__pred, std::invoke(__proj, std::forward<decltype(__value)>(__value)));
                            });
  }

  template <class _Ep, random_access_range _Rp, class _Proj = identity,
            indirect_unary_predicate<projected<iterator_t<_Rp>, _Proj>> _Pred,
            class _RawPolicy = __remove_cvref_t<_Ep>, enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires sized_range<_Rp>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI borrowed_iterator_t<_Rp>
  operator()(_Ep&& __exec, _Rp&& __range, _Pred __pred, _Proj __proj = {}) const {
    return (*this)(std::forward<_Ep>(__exec), ranges::begin(__range), ranges::end(__range), std::move(__pred), std::move(__proj));
  }
#  endif
};

inline namespace __cpo {
inline constexpr auto find_if_not = __find_if_not{};
} // namespace __cpo
} // namespace ranges

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_STD_VER >= 20

_LIBCPP_POP_MACROS

#endif // _LIBCPP___ALGORITHM_RANGES_FIND_IF_NOT_H
