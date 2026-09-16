//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___ALGORITHM_RANGES_NTH_ELEMENT_H
#define _LIBCPP___ALGORITHM_RANGES_NTH_ELEMENT_H

#include <__algorithm/iterator_operations.h>
#include <__algorithm/make_projected.h>
#include <__algorithm/nth_element.h>
#include <__algorithm/pstl.h>
#include <__config>
#include <__functional/identity.h>
#include <__functional/invoke.h>
#include <__functional/ranges_operations.h>
#include <__iterator/concepts.h>
#include <__iterator/iterator_traits.h>
#include <__iterator/next.h>
#include <__iterator/projected.h>
#include <__iterator/sortable.h>
#include <__ranges/access.h>
#include <__ranges/concepts.h>
#include <__ranges/dangling.h>
#include <__utility/forward.h>
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
struct __nth_element {
  template <class _Iter, class _Sent, class _Comp, class _Proj>
  _LIBCPP_HIDE_FROM_ABI constexpr static _Iter
  __nth_element_fn_impl(_Iter __first, _Iter __nth, _Sent __last, _Comp& __comp, _Proj& __proj) {
    auto __last_iter = ranges::next(__first, __last);

    auto&& __projected_comp = std::__make_projected(__comp, __proj);
    std::__nth_element_impl<_RangeAlgPolicy>(std::move(__first), std::move(__nth), __last_iter, __projected_comp);

    return __last_iter;
  }

  template <random_access_iterator _Iter, sentinel_for<_Iter> _Sent, class _Comp = ranges::less, class _Proj = identity>
    requires sortable<_Iter, _Comp, _Proj>
  _LIBCPP_HIDE_FROM_ABI constexpr _Iter
  operator()(_Iter __first, _Iter __nth, _Sent __last, _Comp __comp = {}, _Proj __proj = {}) const {
    return __nth_element_fn_impl(std::move(__first), std::move(__nth), std::move(__last), __comp, __proj);
  }

  template <random_access_range _Range, class _Comp = ranges::less, class _Proj = identity>
    requires sortable<iterator_t<_Range>, _Comp, _Proj>
  _LIBCPP_HIDE_FROM_ABI constexpr borrowed_iterator_t<_Range>
  operator()(_Range&& __r, iterator_t<_Range> __nth, _Comp __comp = {}, _Proj __proj = {}) const {
    return __nth_element_fn_impl(ranges::begin(__r), std::move(__nth), ranges::end(__r), __comp, __proj);
  }

#  if _LIBCPP_HAS_EXPERIMENTAL_PSTL
  template <class _Ep, random_access_iterator _Iter, sized_sentinel_for<_Iter> _Sent,
            class _Comp = ranges::less, class _Proj = identity,
            class _RawPolicy = __remove_cvref_t<_Ep>, enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires sortable<_Iter, _Comp, _Proj>
  _LIBCPP_HIDE_FROM_ABI _Iter operator()(
      _Ep&& __exec, _Iter __first, _Iter __nth, _Sent __last, _Comp __comp = {}, _Proj __proj = {}) const {
    _Iter __end = __first + (__last - __first);
    std::nth_element(std::forward<_Ep>(__exec), __first, __nth, __end,
                     [&__comp, &__proj](auto&& __a, auto&& __b) {
                       return std::invoke(__comp, std::invoke(__proj, std::forward<decltype(__a)>(__a)),
                                          std::invoke(__proj, std::forward<decltype(__b)>(__b)));
                     });
    return __end;
  }

  template <class _Ep, random_access_range _Range, class _Comp = ranges::less, class _Proj = identity,
            class _RawPolicy = __remove_cvref_t<_Ep>, enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires sized_range<_Range> && sortable<iterator_t<_Range>, _Comp, _Proj>
  _LIBCPP_HIDE_FROM_ABI borrowed_iterator_t<_Range> operator()(
      _Ep&& __exec, _Range&& __r, iterator_t<_Range> __nth, _Comp __comp = {}, _Proj __proj = {}) const {
    return (*this)(std::forward<_Ep>(__exec), ranges::begin(__r), std::move(__nth), ranges::end(__r),
                   std::move(__comp), std::move(__proj));
  }
#  endif
};

inline namespace __cpo {
inline constexpr auto nth_element = __nth_element{};
} // namespace __cpo
} // namespace ranges

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_STD_VER >= 20

_LIBCPP_POP_MACROS

#endif // _LIBCPP___ALGORITHM_RANGES_NTH_ELEMENT_H
