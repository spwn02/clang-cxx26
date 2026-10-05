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
#include <__algorithm/in_out_result.h>
#include <__algorithm/iterator_operations.h>
#include <__algorithm/pstl.h>
#include <__config>
#include <__iterator/concepts.h>
#include <__iterator/incrementable_traits.h>
#include <__pstl/ranges_bounded.h>
#include <__type_traits/is_execution_policy.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/forward.h>
#include <__utility/move.h>

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
#  if _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_EXPERIMENTAL_PSTL && !defined(_LIBCPP_FREESTANDING)
  template <class _Ep,
            random_access_iterator _InIter,
            random_access_iterator _OutIter,
            sized_sentinel_for<_OutIter> _OutSent,
            class _RawPolicy = __remove_cvref_t<_Ep>,
            enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires indirectly_copyable<_InIter, _OutIter>
  _LIBCPP_HIDE_FROM_ABI copy_n_result<_InIter, _OutIter>
  operator()(_Ep&& __exec, _InIter __first, iter_difference_t<_InIter> __n, _OutIter __result, _OutSent __result_last) const {
    using _Implementation = __pstl::__dispatch<__pstl::__ranges_bounded_copy_n, __pstl::__current_configuration, _RawPolicy>;
    return __pstl::__handle_exception<_Implementation>(std::forward<_Ep>(__exec), std::move(__first), __n, std::move(__result), std::move(__result_last));
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
