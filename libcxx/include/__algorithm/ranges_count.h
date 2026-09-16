//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___ALGORITHM_RANGES_COUNT_H
#define _LIBCPP___ALGORITHM_RANGES_COUNT_H

#include <__algorithm/count.h>
#include <__algorithm/count_if.h>
#include <__algorithm/iterator_operations.h>
#include <__algorithm/pstl.h>
#include <__config>
#include <__functional/identity.h>
#include <__functional/ranges_operations.h>
#include <__iterator/concepts.h>
#include <__iterator/incrementable_traits.h>
#include <__iterator/iterator_traits.h>
#include <__iterator/projected.h>
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

#if _LIBCPP_STD_VER >= 20

_LIBCPP_BEGIN_NAMESPACE_STD

namespace ranges {
struct __count {
  template <input_iterator _Iter,
            sentinel_for<_Iter> _Sent,
            class _Proj = identity,
            class _Type
#  if _LIBCPP_STD_VER >= 26
            = projected_value_t<_Iter, _Proj>
#  endif
            >
    requires indirect_binary_predicate<ranges::equal_to, projected<_Iter, _Proj>, const _Type*>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr iter_difference_t<_Iter>
  operator()(_Iter __first, _Sent __last, const _Type& __value, _Proj __proj = {}) const {
    return std::__count<_RangeAlgPolicy>(std::move(__first), std::move(__last), __value, __proj);
  }

  template <input_range _Range,
            class _Proj = identity,
            class _Type
#  if _LIBCPP_STD_VER >= 26
            = projected_value_t<iterator_t<_Range>, _Proj>
#  endif
            >
    requires indirect_binary_predicate<ranges::equal_to, projected<iterator_t<_Range>, _Proj>, const _Type*>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr range_difference_t<_Range>
  operator()(_Range&& __r, const _Type& __value, _Proj __proj = {}) const {
    return std::__count<_RangeAlgPolicy>(ranges::begin(__r), ranges::end(__r), __value, __proj);
  }

#  if _LIBCPP_HAS_EXPERIMENTAL_PSTL
  template <class _Ep, random_access_iterator _Iter, sized_sentinel_for<_Iter> _Sent,
            class _Proj = identity, class _Type
#    if _LIBCPP_STD_VER >= 26
            = projected_value_t<_Iter, _Proj>
#    endif
            , class _RawPolicy = __remove_cvref_t<_Ep>, enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires indirect_binary_predicate<ranges::equal_to, projected<_Iter, _Proj>, const _Type*>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI iter_difference_t<_Iter>
  operator()(_Ep&& __exec, _Iter __first, _Sent __last, const _Type& __value, _Proj __proj = {}) const {
    _Iter __end = __first + (__last - __first);
    return std::count_if(std::forward<_Ep>(__exec), std::move(__first), std::move(__end),
                         [&__value, __proj = std::move(__proj)](auto&& __element) mutable {
                           return std::invoke(__proj, std::forward<decltype(__element)>(__element)) == __value;
                         });
  }

  template <class _Ep, random_access_range _Range, class _Proj = identity, class _Type
#    if _LIBCPP_STD_VER >= 26
            = projected_value_t<iterator_t<_Range>, _Proj>
#    endif
            , class _RawPolicy = __remove_cvref_t<_Ep>, enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires sized_range<_Range> && indirect_binary_predicate<ranges::equal_to, projected<iterator_t<_Range>, _Proj>, const _Type*>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI range_difference_t<_Range>
  operator()(_Ep&& __exec, _Range&& __range, const _Type& __value, _Proj __proj = {}) const {
    return (*this)(std::forward<_Ep>(__exec), ranges::begin(__range), ranges::end(__range), __value, std::move(__proj));
  }
#  endif
};

inline namespace __cpo {
inline constexpr auto count = __count{};
} // namespace __cpo
} // namespace ranges

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_STD_VER >= 20

_LIBCPP_POP_MACROS

#endif // _LIBCPP___ALGORITHM_RANGES_COUNT_H
