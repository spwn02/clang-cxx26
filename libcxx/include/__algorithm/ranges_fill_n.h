//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___ALGORITHM_RANGES_FILL_N_H
#define _LIBCPP___ALGORITHM_RANGES_FILL_N_H

#include <__algorithm/fill_n.h>
#include <__algorithm/pstl.h>
#include <__config>
#include <__iterator/concepts.h>
#include <__iterator/incrementable_traits.h>
#include <__iterator/iterator_traits.h>
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
struct __fill_n {
  template <class _Iter,
            class _Type
#  if _LIBCPP_STD_VER >= 26
            = iter_value_t<_Iter>
#  endif
            >
    requires output_iterator<_Iter, const _Type&>
  _LIBCPP_HIDE_FROM_ABI constexpr _Iter
  operator()(_Iter __first, iter_difference_t<_Iter> __n, const _Type& __value) const {
    return std::__fill_n(std::move(__first), __n, __value);
  }

#  if _LIBCPP_HAS_EXPERIMENTAL_PSTL
  template <class _Ep, random_access_iterator _Iter, class _Type
#    if _LIBCPP_STD_VER >= 26
            = iter_value_t<_Iter>
#    endif
            , class _RawPolicy = __remove_cvref_t<_Ep>, enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires output_iterator<_Iter, const _Type&>
  _LIBCPP_HIDE_FROM_ABI _Iter
  operator()(_Ep&& __exec, _Iter __first, iter_difference_t<_Iter> __n, const _Type& __value) const {
    _Iter __end = __first + __n;
    std::fill_n(std::forward<_Ep>(__exec), std::move(__first), __n, __value);
    return __end;
  }
#  endif
};

inline namespace __cpo {
inline constexpr auto fill_n = __fill_n{};
} // namespace __cpo
} // namespace ranges

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_STD_VER >= 20

_LIBCPP_POP_MACROS

#endif // _LIBCPP___ALGORITHM_RANGES_FILL_N_H
