//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP__ALGORITHM_RANGES_IS_SORTED_H
#define _LIBCPP__ALGORITHM_RANGES_IS_SORTED_H

#include <__algorithm/ranges_is_sorted_until.h>
#include <__algorithm/pstl.h>
#include <__config>
#include <__functional/identity.h>
#include <__functional/invoke.h>
#include <__functional/ranges_operations.h>
#include <__iterator/concepts.h>
#include <__iterator/projected.h>
#include <__ranges/access.h>
#include <__ranges/concepts.h>
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
struct __is_sorted {
  template <forward_iterator _Iter,
            sentinel_for<_Iter> _Sent,
            class _Proj                                               = identity,
            indirect_strict_weak_order<projected<_Iter, _Proj>> _Comp = ranges::less>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr bool
  operator()(_Iter __first, _Sent __last, _Comp __comp = {}, _Proj __proj = {}) const {
    return ranges::__is_sorted_until_impl(std::move(__first), __last, __comp, __proj) == __last;
  }

  template <forward_range _Range,
            class _Proj                                                            = identity,
            indirect_strict_weak_order<projected<iterator_t<_Range>, _Proj>> _Comp = ranges::less>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr bool
  operator()(_Range&& __range, _Comp __comp = {}, _Proj __proj = {}) const {
    auto __last = ranges::end(__range);
    return ranges::__is_sorted_until_impl(ranges::begin(__range), __last, __comp, __proj) == __last;
  }

#  if _LIBCPP_HAS_EXPERIMENTAL_PSTL
  template <class _Ep, random_access_iterator _Iter, sized_sentinel_for<_Iter> _Sent,
            class _Proj = identity,
            indirect_strict_weak_order<projected<_Iter, _Proj>> _Comp = ranges::less,
            class _RawPolicy = __remove_cvref_t<_Ep>, enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI bool operator()(
      _Ep&& __exec, _Iter __first, _Sent __last, _Comp __comp = {}, _Proj __proj = {}) const {
    _Iter __end = __first + (__last - __first);
    return std::is_sorted(std::forward<_Ep>(__exec), __first, __end,
                          [&__comp, &__proj](auto&& __a, auto&& __b) {
                            return std::invoke(__comp, std::invoke(__proj, std::forward<decltype(__a)>(__a)),
                                               std::invoke(__proj, std::forward<decltype(__b)>(__b)));
                          });
  }

  template <class _Ep, random_access_range _Range, class _Proj = identity,
            indirect_strict_weak_order<projected<iterator_t<_Range>, _Proj>> _Comp = ranges::less,
            class _RawPolicy = __remove_cvref_t<_Ep>, enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires sized_range<_Range>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI bool operator()(
      _Ep&& __exec, _Range&& __range, _Comp __comp = {}, _Proj __proj = {}) const {
    return (*this)(std::forward<_Ep>(__exec), ranges::begin(__range), ranges::end(__range),
                   std::move(__comp), std::move(__proj));
  }
#  endif
};

inline namespace __cpo {
inline constexpr auto is_sorted = __is_sorted{};
} // namespace __cpo
} // namespace ranges

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_STD_VER >= 20

_LIBCPP_POP_MACROS

#endif // _LIBCPP__ALGORITHM_RANGES_IS_SORTED_H
