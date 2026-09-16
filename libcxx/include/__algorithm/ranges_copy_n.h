//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___ALGORITHM_RANGES_COPY_N_H
#define _LIBCPP___ALGORITHM_RANGES_COPY_N_H

#include <__algorithm/copy_n.h>
#include <__algorithm/pstl.h>
#include <__algorithm/in_out_result.h>
#include <__algorithm/iterator_operations.h>
#include <__config>
#include <__iterator/concepts.h>
#include <__iterator/incrementable_traits.h>
#include <__utility/move.h>
#include <__utility/forward.h>
#include <__type_traits/is_execution_policy.h>
#include <__type_traits/remove_cvref.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 20

namespace ranges {

template <class _Ip, class _Op>
using copy_n_result = in_out_result<_Ip, _Op>;

struct __copy_n {
  template <input_iterator _Ip, weakly_incrementable _Op>
    requires indirectly_copyable<_Ip, _Op>
  _LIBCPP_HIDE_FROM_ABI constexpr copy_n_result<_Ip, _Op>
  operator()(_Ip __first, iter_difference_t<_Ip> __n, _Op __result) const {
    auto __res = std::__copy_n<_RangeAlgPolicy>(std::move(__first), __n, std::move(__result));
    return {std::move(__res.first), std::move(__res.second)};
  }
#  if _LIBCPP_HAS_EXPERIMENTAL_PSTL
  template <class _Ep, random_access_iterator _Ip, weakly_incrementable _Op,
            class _RawPolicy = __remove_cvref_t<_Ep>, enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires indirectly_copyable<_Ip, _Op>
  _LIBCPP_HIDE_FROM_ABI copy_n_result<_Ip, _Op>
  operator()(_Ep&& __exec, _Ip __first, iter_difference_t<_Ip> __n, _Op __result) const {
    _Ip __end = __first + __n;
    auto __out = std::copy_n(std::forward<_Ep>(__exec), std::move(__first), __n, std::move(__result));
    return {std::move(__end), std::move(__out)};
  }
#  endif
};

inline namespace __cpo {
inline constexpr auto copy_n = __copy_n{};
} // namespace __cpo
} // namespace ranges

#endif // _LIBCPP_STD_VER >= 20

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP___ALGORITHM_RANGES_COPY_N_H
