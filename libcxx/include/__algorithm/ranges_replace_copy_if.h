//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___ALGORITHM_RANGES_REPLACE_COPY_IF_H
#define _LIBCPP___ALGORITHM_RANGES_REPLACE_COPY_IF_H

#include <__algorithm/in_out_result.h>
#include <__algorithm/pstl.h>
#include <__config>
#include <__functional/identity.h>
#include <__functional/invoke.h>
#include <__iterator/concepts.h>
#include <__iterator/iterator_traits.h>
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

template <class _InIter, class _OutIter>
using replace_copy_if_result = in_out_result<_InIter, _OutIter>;

template <class _InIter, class _Sent, class _OutIter, class _Pred, class _Type, class _Proj>
_LIBCPP_HIDE_FROM_ABI constexpr replace_copy_if_result<_InIter, _OutIter> __replace_copy_if_impl(
    _InIter __first, _Sent __last, _OutIter __result, _Pred& __pred, const _Type& __new_value, _Proj& __proj) {
  while (__first != __last) {
    if (std::invoke(__pred, std::invoke(__proj, *__first)))
      *__result = __new_value;
    else
      *__result = *__first;

    ++__first;
    ++__result;
  }

  return {std::move(__first), std::move(__result)};
}

struct __replace_copy_if {
  template <input_iterator _InIter,
            sentinel_for<_InIter> _Sent,
            class _OutIter,
            class _Type
#  if _LIBCPP_STD_VER >= 26
            = iter_value_t<_OutIter>
#  endif
            ,
            class _Proj = identity,
            indirect_unary_predicate<projected<_InIter, _Proj>> _Pred>
    requires indirectly_copyable<_InIter, _OutIter> && output_iterator<_OutIter, const _Type&>
  _LIBCPP_HIDE_FROM_ABI constexpr replace_copy_if_result<_InIter, _OutIter> operator()(
      _InIter __first, _Sent __last, _OutIter __result, _Pred __pred, const _Type& __new_value, _Proj __proj = {})
      const {
    return ranges::__replace_copy_if_impl(
        std::move(__first), std::move(__last), std::move(__result), __pred, __new_value, __proj);
  }

  template <input_range _Range,
            class _OutIter,
            class _Type
#  if _LIBCPP_STD_VER >= 26
            = iter_value_t<_OutIter>
#  endif
            ,
            class _Proj = identity,
            indirect_unary_predicate<projected<iterator_t<_Range>, _Proj>> _Pred>
    requires indirectly_copyable<iterator_t<_Range>, _OutIter> && output_iterator<_OutIter, const _Type&>
  _LIBCPP_HIDE_FROM_ABI constexpr replace_copy_if_result<borrowed_iterator_t<_Range>, _OutIter>
  operator()(_Range&& __range, _OutIter __result, _Pred __pred, const _Type& __new_value, _Proj __proj = {}) const {
    return ranges::__replace_copy_if_impl(
        ranges::begin(__range), ranges::end(__range), std::move(__result), __pred, __new_value, __proj);
  }

#  if _LIBCPP_HAS_EXPERIMENTAL_PSTL
  template <class _Ep, random_access_iterator _InIter, sized_sentinel_for<_InIter> _Sent, class _OutIter, class _Type,
            class _Proj = identity, indirect_unary_predicate<projected<_InIter, _Proj>> _Pred,
            class _RawPolicy = __remove_cvref_t<_Ep>, enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires indirectly_copyable<_InIter, _OutIter> && output_iterator<_OutIter, const _Type&>
  _LIBCPP_HIDE_FROM_ABI replace_copy_if_result<_InIter, _OutIter> operator()(
      _Ep&& __exec, _InIter __first, _Sent __last, _OutIter __result, _Pred __pred, const _Type& __new_value,
      _Proj __proj = {}) const {
    auto __count = __last - __first;
    _InIter __end = __first + __count;
    _OutIter __out = __result;
    std::replace_copy_if(std::forward<_Ep>(__exec), std::move(__first), __end, std::move(__result),
                         [__pred = std::move(__pred), __proj = std::move(__proj)](auto&& __value) mutable {
                           return std::invoke(__pred, std::invoke(__proj, std::forward<decltype(__value)>(__value)));
                         }, __new_value);
    for (decltype(__count) __i = 0; __i != __count; ++__i)
      ++__out;
    return {std::move(__end), std::move(__out)};
  }

  template <class _Ep, random_access_range _Range, class _OutIter, class _Type, class _Proj = identity,
            indirect_unary_predicate<projected<iterator_t<_Range>, _Proj>> _Pred,
            class _RawPolicy = __remove_cvref_t<_Ep>, enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires sized_range<_Range> && indirectly_copyable<iterator_t<_Range>, _OutIter> && output_iterator<_OutIter, const _Type&>
  _LIBCPP_HIDE_FROM_ABI replace_copy_if_result<borrowed_iterator_t<_Range>, _OutIter> operator()(
      _Ep&& __exec, _Range&& __range, _OutIter __result, _Pred __pred, const _Type& __new_value, _Proj __proj = {}) const {
    return (*this)(std::forward<_Ep>(__exec), ranges::begin(__range), ranges::end(__range), std::move(__result), std::move(__pred), __new_value, std::move(__proj));
  }
#  endif
};

inline namespace __cpo {
inline constexpr auto replace_copy_if = __replace_copy_if{};
} // namespace __cpo
} // namespace ranges

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_STD_VER >= 20

_LIBCPP_POP_MACROS

#endif // _LIBCPP___ALGORITHM_RANGES_REPLACE_COPY_IF_H
