//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___ALGORITHM_RANGES_FIND_H
#define _LIBCPP___ALGORITHM_RANGES_FIND_H

#include <__algorithm/find.h>
#include <__algorithm/ranges_find_if.h>
#include <__algorithm/pstl.h>
#include <__algorithm/unwrap_range.h>
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
struct __find {
  template <class _Iter, class _Sent, class _Tp, class _Proj>
  _LIBCPP_HIDE_FROM_ABI static constexpr _Iter
  __find_unwrap(_Iter __first, _Sent __last, const _Tp& __value, _Proj& __proj) {
    if constexpr (forward_iterator<_Iter>) {
      auto [__first_un, __last_un] = std::__unwrap_range(__first, std::move(__last));
      return std::__rewrap_range<_Sent>(
          std::move(__first), std::__find(std::move(__first_un), std::move(__last_un), __value, __proj));
    } else {
      return std::__find(std::move(__first), std::move(__last), __value, __proj);
    }
  }

  template <input_iterator _Ip,
            sentinel_for<_Ip> _Sp,
            class _Proj = identity,
            class _Tp
#  if _LIBCPP_STD_VER >= 26
            = projected_value_t<_Ip, _Proj>
#  endif
            >
    requires indirect_binary_predicate<ranges::equal_to, projected<_Ip, _Proj>, const _Tp*>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr _Ip
  operator()(_Ip __first, _Sp __last, const _Tp& __value, _Proj __proj = {}) const {
    return __find_unwrap(std::move(__first), std::move(__last), __value, __proj);
  }

  template <input_range _Rp,
            class _Proj = identity,
            class _Tp
#  if _LIBCPP_STD_VER >= 26
            = projected_value_t<iterator_t<_Rp>, _Proj>
#  endif
            >
    requires indirect_binary_predicate<ranges::equal_to, projected<iterator_t<_Rp>, _Proj>, const _Tp*>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr borrowed_iterator_t<_Rp>
  operator()(_Rp&& __r, const _Tp& __value, _Proj __proj = {}) const {
    return __find_unwrap(ranges::begin(__r), ranges::end(__r), __value, __proj);
  }

#  if _LIBCPP_HAS_EXPERIMENTAL_PSTL
  template <class _Ep, random_access_iterator _Ip, sized_sentinel_for<_Ip> _Sp,
            class _Proj = identity, class _Tp
#    if _LIBCPP_STD_VER >= 26
            = projected_value_t<_Ip, _Proj>
#    endif
            , class _RawPolicy = __remove_cvref_t<_Ep>, enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires indirect_binary_predicate<ranges::equal_to, projected<_Ip, _Proj>, const _Tp*>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI _Ip
  operator()(_Ep&& __exec, _Ip __first, _Sp __last, const _Tp& __value, _Proj __proj = {}) const {
    _Ip __end = __first + (__last - __first);
    // std::find(policy, first, last, value) compares by operator== against value directly and
    // takes no predicate -- a projection needs std::find_if instead.
    return std::find_if(std::forward<_Ep>(__exec), std::move(__first), std::move(__end),
                        [&__value, __proj = std::move(__proj)](auto&& __element) mutable {
                          return std::invoke(__proj, std::forward<decltype(__element)>(__element)) == __value;
                        });
  }

  template <class _Ep, random_access_range _Rp, class _Proj = identity, class _Tp
#    if _LIBCPP_STD_VER >= 26
            = projected_value_t<iterator_t<_Rp>, _Proj>
#    endif
            , class _RawPolicy = __remove_cvref_t<_Ep>, enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires sized_range<_Rp> && indirect_binary_predicate<ranges::equal_to, projected<iterator_t<_Rp>, _Proj>, const _Tp*>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI borrowed_iterator_t<_Rp>
  operator()(_Ep&& __exec, _Rp&& __range, const _Tp& __value, _Proj __proj = {}) const {
    return (*this)(std::forward<_Ep>(__exec), ranges::begin(__range), ranges::end(__range), __value, std::move(__proj));
  }
#  endif
};

inline namespace __cpo {
inline constexpr auto find = __find{};
} // namespace __cpo
} // namespace ranges

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_STD_VER >= 20

_LIBCPP_POP_MACROS

#endif // _LIBCPP___ALGORITHM_RANGES_FIND_H
