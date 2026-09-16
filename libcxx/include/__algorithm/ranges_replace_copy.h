//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___ALGORITHM_RANGES_REPLACE_COPY_H
#define _LIBCPP___ALGORITHM_RANGES_REPLACE_COPY_H

#include <__algorithm/in_out_result.h>
#include <__algorithm/pstl.h>
#include <__algorithm/ranges_replace_copy_if.h>
#include <__config>
#include <__functional/identity.h>
#include <__functional/invoke.h>
#include <__functional/ranges_operations.h>
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
using replace_copy_result = in_out_result<_InIter, _OutIter>;

struct __replace_copy {
  template <input_iterator _InIter,
            sentinel_for<_InIter> _Sent,
            class _OutIter,
            class _Proj = identity,
            class _OldType
#  if _LIBCPP_STD_VER >= 26
            = projected_value_t<_InIter, _Proj>
#  endif
            ,
            class _NewType
#  if _LIBCPP_STD_VER >= 26
            = iter_value_t<_OutIter>
#  endif
            >
    requires indirectly_copyable<_InIter, _OutIter> &&
             indirect_binary_predicate<ranges::equal_to, projected<_InIter, _Proj>, const _OldType*> &&
             output_iterator<_OutIter, const _NewType&>
  _LIBCPP_HIDE_FROM_ABI constexpr replace_copy_result<_InIter, _OutIter>
  operator()(_InIter __first,
             _Sent __last,
             _OutIter __result,
             const _OldType& __old_value,
             const _NewType& __new_value,
             _Proj __proj = {}) const {
    auto __pred = [&](const auto& __value) -> bool { return __value == __old_value; };
    return ranges::__replace_copy_if_impl(
        std::move(__first), std::move(__last), std::move(__result), __pred, __new_value, __proj);
  }

  template <input_range _Range,
            class _OutIter,
            class _Proj = identity,
            class _OldType
#  if _LIBCPP_STD_VER >= 26
            = projected_value_t<iterator_t<_Range>, _Proj>
#  endif
            ,
            class _NewType
#  if _LIBCPP_STD_VER >= 26
            = iter_value_t<_OutIter>
#  endif
            >
    requires indirectly_copyable<iterator_t<_Range>, _OutIter> &&
             indirect_binary_predicate<ranges::equal_to, projected<iterator_t<_Range>, _Proj>, const _OldType*> &&
             output_iterator<_OutIter, const _NewType&>
  _LIBCPP_HIDE_FROM_ABI constexpr replace_copy_result<borrowed_iterator_t<_Range>, _OutIter> operator()(
      _Range&& __range, _OutIter __result, const _OldType& __old_value, const _NewType& __new_value, _Proj __proj = {})
      const {
    auto __pred = [&](const auto& __value) -> bool { return __value == __old_value; };
    return ranges::__replace_copy_if_impl(
        ranges::begin(__range), ranges::end(__range), std::move(__result), __pred, __new_value, __proj);
  }

#  if _LIBCPP_HAS_EXPERIMENTAL_PSTL
  template <class _Ep, random_access_iterator _InIter, sized_sentinel_for<_InIter> _Sent, class _OutIter,
            class _Proj = identity, class _OldType, class _NewType, class _RawPolicy = __remove_cvref_t<_Ep>,
            enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires indirectly_copyable<_InIter, _OutIter> &&
             indirect_binary_predicate<ranges::equal_to, projected<_InIter, _Proj>, const _OldType*> &&
             output_iterator<_OutIter, const _NewType&>
  _LIBCPP_HIDE_FROM_ABI replace_copy_result<_InIter, _OutIter> operator()(
      _Ep&& __exec, _InIter __first, _Sent __last, _OutIter __result, const _OldType& __old_value,
      const _NewType& __new_value, _Proj __proj = {}) const {
    auto __count = __last - __first;
    _InIter __end = __first + __count;
    _OutIter __out = __result;
    std::replace_copy_if(std::forward<_Ep>(__exec), std::move(__first), __end, std::move(__result),
                         [&__old_value, __proj = std::move(__proj)](auto&& __value) mutable {
                           return std::invoke(__proj, std::forward<decltype(__value)>(__value)) == __old_value;
                         }, __new_value);
    for (decltype(__count) __i = 0; __i != __count; ++__i)
      ++__out;
    return {std::move(__end), std::move(__out)};
  }

  template <class _Ep, random_access_range _Range, class _OutIter, class _Proj = identity, class _OldType,
            class _NewType, class _RawPolicy = __remove_cvref_t<_Ep>, enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires sized_range<_Range> && indirectly_copyable<iterator_t<_Range>, _OutIter> &&
             indirect_binary_predicate<ranges::equal_to, projected<iterator_t<_Range>, _Proj>, const _OldType*> &&
             output_iterator<_OutIter, const _NewType&>
  _LIBCPP_HIDE_FROM_ABI replace_copy_result<borrowed_iterator_t<_Range>, _OutIter> operator()(
      _Ep&& __exec, _Range&& __range, _OutIter __result, const _OldType& __old_value, const _NewType& __new_value,
      _Proj __proj = {}) const {
    return (*this)(std::forward<_Ep>(__exec), ranges::begin(__range), ranges::end(__range), std::move(__result), __old_value, __new_value, std::move(__proj));
  }
#  endif
};

inline namespace __cpo {
inline constexpr auto replace_copy = __replace_copy{};
} // namespace __cpo
} // namespace ranges

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_STD_VER >= 20

_LIBCPP_POP_MACROS

#endif // _LIBCPP___ALGORITHM_RANGES_REPLACE_COPY_H
