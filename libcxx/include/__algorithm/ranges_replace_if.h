//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___ALGORITHM_RANGES_REPLACE_IF_H
#define _LIBCPP___ALGORITHM_RANGES_REPLACE_IF_H

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

template <class _Iter, class _Sent, class _Type, class _Proj, class _Pred>
_LIBCPP_HIDE_FROM_ABI constexpr _Iter
__replace_if_impl(_Iter __first, _Sent __last, _Pred& __pred, const _Type& __new_value, _Proj& __proj) {
  for (; __first != __last; ++__first) {
    if (std::invoke(__pred, std::invoke(__proj, *__first)))
      *__first = __new_value;
  }
  return __first;
}

struct __replace_if {
  template <input_iterator _Iter,
            sentinel_for<_Iter> _Sent,
            class _Proj = identity,
            class _Type
#  if _LIBCPP_STD_VER >= 26
            = projected_value_t<_Iter, _Proj>
#  endif
            ,
            indirect_unary_predicate<projected<_Iter, _Proj>> _Pred>
    requires indirectly_writable<_Iter, const _Type&>
  _LIBCPP_HIDE_FROM_ABI constexpr _Iter
  operator()(_Iter __first, _Sent __last, _Pred __pred, const _Type& __new_value, _Proj __proj = {}) const {
    return ranges::__replace_if_impl(std::move(__first), std::move(__last), __pred, __new_value, __proj);
  }

  template <input_range _Range,
            class _Proj = identity,
            class _Type
#  if _LIBCPP_STD_VER >= 26
            = projected_value_t<iterator_t<_Range>, _Proj>
#  endif
            ,
            indirect_unary_predicate<projected<iterator_t<_Range>, _Proj>> _Pred>
    requires indirectly_writable<iterator_t<_Range>, const _Type&>
  _LIBCPP_HIDE_FROM_ABI constexpr borrowed_iterator_t<_Range>
  operator()(_Range&& __range, _Pred __pred, const _Type& __new_value, _Proj __proj = {}) const {
    return ranges::__replace_if_impl(ranges::begin(__range), ranges::end(__range), __pred, __new_value, __proj);
  }

#  if _LIBCPP_HAS_EXPERIMENTAL_PSTL
  template <class _Ep, random_access_iterator _Iter, sized_sentinel_for<_Iter> _Sent, class _Proj = identity,
            class _Type, indirect_unary_predicate<projected<_Iter, _Proj>> _Pred,
            class _RawPolicy = __remove_cvref_t<_Ep>, enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires indirectly_writable<_Iter, const _Type&>
  _LIBCPP_HIDE_FROM_ABI _Iter operator()(
      _Ep&& __exec, _Iter __first, _Sent __last, _Pred __pred, const _Type& __new_value, _Proj __proj = {}) const {
    _Iter __end = __first + (__last - __first);
    std::replace_if(std::forward<_Ep>(__exec), std::move(__first), __end,
                    [__pred = std::move(__pred), __proj = std::move(__proj)](auto&& __value) mutable {
                      return std::invoke(__pred, std::invoke(__proj, std::forward<decltype(__value)>(__value)));
                    }, __new_value);
    return __end;
  }

  template <class _Ep, random_access_range _Range, class _Type, class _Proj = identity,
            indirect_unary_predicate<projected<iterator_t<_Range>, _Proj>> _Pred,
            class _RawPolicy = __remove_cvref_t<_Ep>, enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires sized_range<_Range> && indirectly_writable<iterator_t<_Range>, const _Type&>
  _LIBCPP_HIDE_FROM_ABI borrowed_iterator_t<_Range> operator()(
      _Ep&& __exec, _Range&& __range, _Pred __pred, const _Type& __new_value, _Proj __proj = {}) const {
    return (*this)(std::forward<_Ep>(__exec), ranges::begin(__range), ranges::end(__range), std::move(__pred), __new_value, std::move(__proj));
  }
#  endif
};

inline namespace __cpo {
inline constexpr auto replace_if = __replace_if{};
} // namespace __cpo
} // namespace ranges

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_STD_VER >= 20

_LIBCPP_POP_MACROS

#endif // _LIBCPP___ALGORITHM_RANGES_REPLACE_IF_H
