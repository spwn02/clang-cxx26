//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___ALGORITHM_RANGES_GENERATE_H
#define _LIBCPP___ALGORITHM_RANGES_GENERATE_H

#include <__concepts/constructible.h>
#include <__concepts/invocable.h>
#include <__algorithm/pstl.h>
#include <__config>
#include <__iterator/concepts.h>
#include <__iterator/iterator_traits.h>
#include <__ranges/access.h>
#include <__ranges/concepts.h>
#include <__ranges/dangling.h>
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
struct __generate {
  template <class _OutIter, class _Sent, class _Func>
  _LIBCPP_HIDE_FROM_ABI constexpr static _OutIter __generate_fn_impl(_OutIter __first, _Sent __last, _Func& __gen) {
    for (; __first != __last; ++__first) {
      *__first = __gen();
    }

    return __first;
  }

  template <input_or_output_iterator _OutIter, sentinel_for<_OutIter> _Sent, copy_constructible _Func>
    requires invocable<_Func&> && indirectly_writable<_OutIter, invoke_result_t<_Func&>>
  _LIBCPP_HIDE_FROM_ABI constexpr _OutIter operator()(_OutIter __first, _Sent __last, _Func __gen) const {
    return __generate_fn_impl(std::move(__first), std::move(__last), __gen);
  }

  template <class _Range, copy_constructible _Func>
    requires invocable<_Func&> && output_range<_Range, invoke_result_t<_Func&>>
  _LIBCPP_HIDE_FROM_ABI constexpr borrowed_iterator_t<_Range> operator()(_Range&& __range, _Func __gen) const {
    return __generate_fn_impl(ranges::begin(__range), ranges::end(__range), __gen);
  }

#  if _LIBCPP_HAS_EXPERIMENTAL_PSTL
  template <class _Ep, random_access_iterator _OutIter, sized_sentinel_for<_OutIter> _Sent, copy_constructible _Func,
            class _RawPolicy = __remove_cvref_t<_Ep>, enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires invocable<_Func&> && indirectly_writable<_OutIter, invoke_result_t<_Func&>>
  _LIBCPP_HIDE_FROM_ABI _OutIter
  operator()(_Ep&& __exec, _OutIter __first, _Sent __last, _Func __gen) const {
    _OutIter __end = __first + (__last - __first);
    std::generate(std::forward<_Ep>(__exec), std::move(__first), __end, std::move(__gen));
    return __end;
  }

  template <class _Ep, random_access_range _Range, copy_constructible _Func,
            class _RawPolicy = __remove_cvref_t<_Ep>, enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires sized_range<_Range> && invocable<_Func&> && output_range<_Range, invoke_result_t<_Func&>>
  _LIBCPP_HIDE_FROM_ABI borrowed_iterator_t<_Range>
  operator()(_Ep&& __exec, _Range&& __range, _Func __gen) const {
    return (*this)(std::forward<_Ep>(__exec), ranges::begin(__range), ranges::end(__range), std::move(__gen));
  }
#  endif
};

inline namespace __cpo {
inline constexpr auto generate = __generate{};
} // namespace __cpo
} // namespace ranges

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_STD_VER >= 20

_LIBCPP_POP_MACROS

#endif // _LIBCPP___ALGORITHM_RANGES_GENERATE_H
