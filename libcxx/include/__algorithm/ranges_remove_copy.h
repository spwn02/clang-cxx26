//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___ALGORITHM_RANGES_REMOVE_COPY_H
#define _LIBCPP___ALGORITHM_RANGES_REMOVE_COPY_H

#include <__algorithm/in_out_result.h>
#include <__algorithm/ranges_remove_copy_if.h>
#include <__config>
#include <__functional/identity.h>
#include <__functional/invoke.h>
#include <__functional/ranges_operations.h>
#include <__iterator/concepts.h>
#include <__iterator/projected.h>
#include <__ranges/access.h>
#include <__ranges/concepts.h>
#include <__ranges/dangling.h>
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
using remove_copy_result = in_out_result<_InIter, _OutIter>;

struct __remove_copy {
  template <input_iterator _InIter,
            sentinel_for<_InIter> _Sent,
            weakly_incrementable _OutIter,
            class _Proj = identity,
            class _Type
#  if _LIBCPP_STD_VER >= 26
            = projected_value_t<_InIter, _Proj>
#  endif
            >
    requires indirectly_copyable<_InIter, _OutIter> &&
             indirect_binary_predicate<ranges::equal_to, projected<_InIter, _Proj>, const _Type*>
  _LIBCPP_HIDE_FROM_ABI constexpr remove_copy_result<_InIter, _OutIter>
  operator()(_InIter __first, _Sent __last, _OutIter __result, const _Type& __value, _Proj __proj = {}) const {
    auto __pred = [&](auto&& __val) -> bool { return __value == __val; };
    return ranges::__remove_copy_if_impl(std::move(__first), std::move(__last), std::move(__result), __pred, __proj);
  }

  template <input_range _Range,
            weakly_incrementable _OutIter,
            class _Proj = identity,
            class _Type
#  if _LIBCPP_STD_VER >= 26
            = projected_value_t<iterator_t<_Range>, _Proj>
#  endif
            >
    requires indirectly_copyable<iterator_t<_Range>, _OutIter> &&
             indirect_binary_predicate<ranges::equal_to, projected<iterator_t<_Range>, _Proj>, const _Type*>
  _LIBCPP_HIDE_FROM_ABI constexpr remove_copy_result<borrowed_iterator_t<_Range>, _OutIter>
  operator()(_Range&& __range, _OutIter __result, const _Type& __value, _Proj __proj = {}) const {
    auto __pred = [&](auto&& __val) -> bool { return __value == __val; };
    return ranges::__remove_copy_if_impl(
        ranges::begin(__range), ranges::end(__range), std::move(__result), __pred, __proj);
  }
#  if _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_EXPERIMENTAL_PSTL && !defined(_LIBCPP_FREESTANDING)
  template <class _Ep,
            random_access_iterator _InIter,
            sized_sentinel_for<_InIter> _Sent,
            random_access_iterator _OutIter,
            sized_sentinel_for<_OutIter> _OutSent,
            class _Proj                                         = identity,
            class _Type                                         = projected_value_t<_InIter, _Proj>,
            class _RawPolicy                                    = __remove_cvref_t<_Ep>,
            enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires indirectly_copyable<_InIter, _OutIter> &&
             indirect_binary_predicate<ranges::equal_to, projected<_InIter, _Proj>, const _Type*>
  _LIBCPP_HIDE_FROM_ABI remove_copy_result<_InIter, _OutIter> operator()(
      _Ep&& __exec,
      _InIter __first,
      _Sent __last,
      _OutIter __result,
      _OutSent __result_last,
      const _Type& __value,
      _Proj __proj = {}) const {
    auto __pred = [&__value](auto&& __elem) { return __elem == __value; };
    return __remove_copy_if{}(
        std::forward<_Ep>(__exec),
        std::move(__first),
        std::move(__last),
        std::move(__result),
        std::move(__result_last),
        __pred,
        std::move(__proj));
  }

  template <class _Ep,
            random_access_range _Range,
            random_access_range _OutRange,
            class _Proj                                         = identity,
            class _Type                                         = projected_value_t<iterator_t<_Range>, _Proj>,
            class _RawPolicy                                    = __remove_cvref_t<_Ep>,
            enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires sized_range<_Range> && sized_range<_OutRange> &&
             indirectly_copyable<iterator_t<_Range>, iterator_t<_OutRange>> &&
             indirect_binary_predicate<ranges::equal_to, projected<iterator_t<_Range>, _Proj>, const _Type*>
  _LIBCPP_HIDE_FROM_ABI remove_copy_result<borrowed_iterator_t<_Range>, borrowed_iterator_t<_OutRange>> operator()(
      _Ep&& __exec, _Range&& __range, _OutRange&& __result_range, const _Type& __value, _Proj __proj = {}) const {
    return (*this)(
        std::forward<_Ep>(__exec),
        ranges::begin(__range),
        ranges::begin(__range) + ranges::size(__range),
        ranges::begin(__result_range),
        ranges::begin(__result_range) + ranges::size(__result_range),
        __value,
        std::move(__proj));
  }
#  endif
};

inline namespace __cpo {
inline constexpr auto remove_copy = __remove_copy{};
} // namespace __cpo
} // namespace ranges

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_STD_VER >= 20

_LIBCPP_POP_MACROS

#endif // _LIBCPP___ALGORITHM_RANGES_REMOVE_COPY_H
