//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___ALGORITHM_RANGES_REMOVE_H
#define _LIBCPP___ALGORITHM_RANGES_REMOVE_H
#include <__config>
#include <__algorithm/pstl.h>
#include <__functional/invoke.h>
#include <__type_traits/is_execution_policy.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/forward.h>

#include <__algorithm/ranges_remove_if.h>
#include <__functional/identity.h>
#include <__functional/ranges_operations.h>
#include <__iterator/concepts.h>
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
struct __remove {
  template <permutable _Iter,
            sentinel_for<_Iter> _Sent,
            class _Proj = identity,
            class _Type
#  if _LIBCPP_STD_VER >= 26
            = projected_value_t<_Iter, _Proj>
#  endif
            >
    requires indirect_binary_predicate<ranges::equal_to, projected<_Iter, _Proj>, const _Type*>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr subrange<_Iter>
  operator()(_Iter __first, _Sent __last, const _Type& __value, _Proj __proj = {}) const {
    auto __pred = [&](auto&& __other) -> bool { return __value == __other; };
    return ranges::__remove_if_impl(std::move(__first), std::move(__last), __pred, __proj);
  }

  template <forward_range _Range,
            class _Proj = identity,
            class _Type
#  if _LIBCPP_STD_VER >= 26
            = projected_value_t<iterator_t<_Range>, _Proj>
#  endif
            >
    requires permutable<iterator_t<_Range>> &&
             indirect_binary_predicate<ranges::equal_to, projected<iterator_t<_Range>, _Proj>, const _Type*>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr borrowed_subrange_t<_Range>
  operator()(_Range&& __range, const _Type& __value, _Proj __proj = {}) const {
    auto __pred = [&](auto&& __other) -> bool { return __value == __other; };
    return ranges::__remove_if_impl(ranges::begin(__range), ranges::end(__range), __pred, __proj);
  }
#  if _LIBCPP_HAS_EXPERIMENTAL_PSTL
  template <class _Ep, random_access_iterator _Iter, sized_sentinel_for<_Iter> _Sent, class _Type,
            class _Proj = identity, class _RawPolicy = __remove_cvref_t<_Ep>,
            enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
  _LIBCPP_HIDE_FROM_ABI subrange<_Iter> operator()(_Ep&& __exec, _Iter __first, _Sent __last, const _Type& __value,
                                                   _Proj __proj = {}) const {
    _Iter __end = __first + (__last - __first);
    _Iter __new_end = std::remove_if(
        std::forward<_Ep>(__exec), __first, __end, [&__value, __proj](auto&& __other) -> bool {
          return __value == std::invoke(__proj, std::forward<decltype(__other)>(__other));
        });
    return {std::move(__new_end), std::move(__end)};
  }
  template <class _Ep, random_access_range _Range, class _Type, class _Proj = identity,
            class _RawPolicy = __remove_cvref_t<_Ep>,
            enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires sized_range<_Range>
  _LIBCPP_HIDE_FROM_ABI borrowed_subrange_t<_Range> operator()(_Ep&& __exec, _Range&& __range, const _Type& __value,
                                                               _Proj __proj = {}) const {
    return (*this)(std::forward<_Ep>(__exec), ranges::begin(__range), ranges::end(__range), __value, std::move(__proj));
  }
#  endif
};

inline namespace __cpo {
inline constexpr auto remove = __remove{};
} // namespace __cpo
} // namespace ranges

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_STD_VER >= 20

_LIBCPP_POP_MACROS

#endif // _LIBCPP___ALGORITHM_RANGES_REMOVE_H
