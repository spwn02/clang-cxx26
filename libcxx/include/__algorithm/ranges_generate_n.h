//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___ALGORITHM_RANGES_GENERATE_N_H
#define _LIBCPP___ALGORITHM_RANGES_GENERATE_N_H

#include <__algorithm/generate_n.h>
#include <__algorithm/pstl.h>
#include <__concepts/constructible.h>
#include <__concepts/invocable.h>
#include <__config>
#include <__functional/identity.h>
#include <__iterator/concepts.h>
#include <__iterator/incrementable_traits.h>
#include <__iterator/iterator_traits.h>
#include <__ranges/access.h>
#include <__ranges/concepts.h>
#include <__type_traits/invoke.h>
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
struct __generate_n {
  template <input_or_output_iterator _OutIter, copy_constructible _Func>
    requires invocable<_Func&> && indirectly_writable<_OutIter, invoke_result_t<_Func&>>
  _LIBCPP_HIDE_FROM_ABI constexpr _OutIter
  operator()(_OutIter __first, iter_difference_t<_OutIter> __n, _Func __gen) const {
    return std::__generate_n(std::move(__first), __n, __gen);
  }

#  if _LIBCPP_HAS_EXPERIMENTAL_PSTL
  template <class _Ep, random_access_iterator _OutIter, copy_constructible _Func,
            class _RawPolicy = __remove_cvref_t<_Ep>, enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires invocable<_Func&> && indirectly_writable<_OutIter, invoke_result_t<_Func&>>
  _LIBCPP_HIDE_FROM_ABI _OutIter
  operator()(_Ep&& __exec, _OutIter __first, iter_difference_t<_OutIter> __n, _Func __gen) const {
    _OutIter __end = __first + __n;
    std::generate_n(std::forward<_Ep>(__exec), std::move(__first), __n, std::move(__gen));
    return __end;
  }
#  endif
};

inline namespace __cpo {
inline constexpr auto generate_n = __generate_n{};
} // namespace __cpo
} // namespace ranges

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_STD_VER >= 20

_LIBCPP_POP_MACROS

#endif // _LIBCPP___ALGORITHM_RANGES_GENERATE_N_H
