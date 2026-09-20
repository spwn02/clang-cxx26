//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___ALGORITHM_RANGES_COPY_H
#define _LIBCPP___ALGORITHM_RANGES_COPY_H

#include <__algorithm/copy.h>
#include <__algorithm/pstl.h>
#include <__algorithm/in_out_result.h>
#include <__config>
#include <__functional/identity.h>
#include <__iterator/concepts.h>
#include <__ranges/access.h>
#include <__ranges/concepts.h>
#include <__ranges/dangling.h>
#include <__utility/move.h>
#include <__utility/forward.h>
#include <__type_traits/is_execution_policy.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/pair.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

#if _LIBCPP_STD_VER >= 20

_LIBCPP_BEGIN_NAMESPACE_STD

namespace ranges {

template <class _InIter, class _OutIter>
using copy_result = in_out_result<_InIter, _OutIter>;

struct __copy {
  template <input_iterator _InIter, sentinel_for<_InIter> _Sent, weakly_incrementable _OutIter>
    requires indirectly_copyable<_InIter, _OutIter>
  _LIBCPP_HIDE_FROM_ABI constexpr copy_result<_InIter, _OutIter>
  operator()(_InIter __first, _Sent __last, _OutIter __result) const {
    auto __ret = std::__copy(std::move(__first), std::move(__last), std::move(__result));
    return {std::move(__ret.first), std::move(__ret.second)};
  }

  template <input_range _Range, weakly_incrementable _OutIter>
    requires indirectly_copyable<iterator_t<_Range>, _OutIter>
  _LIBCPP_HIDE_FROM_ABI constexpr copy_result<borrowed_iterator_t<_Range>, _OutIter>
  operator()(_Range&& __r, _OutIter __result) const {
    auto __ret = std::__copy(ranges::begin(__r), ranges::end(__r), std::move(__result));
    return {std::move(__ret.first), std::move(__ret.second)};
  }
#  if _LIBCPP_HAS_EXPERIMENTAL_PSTL
  template <class _Ep, random_access_iterator _InIter, sized_sentinel_for<_InIter> _Sent, weakly_incrementable _OutIter,
            class _RawPolicy = __remove_cvref_t<_Ep>, enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires indirectly_copyable<_InIter, _OutIter>
  _LIBCPP_HIDE_FROM_ABI copy_result<_InIter, _OutIter>
  operator()(_Ep&& __exec, _InIter __first, _Sent __last, _OutIter __result) const {
    _InIter __end = __first + (__last - __first);
    auto __result_end = std::copy(std::forward<_Ep>(__exec), std::move(__first), __end, std::move(__result));
    return {std::move(__end), std::move(__result_end)};
  }

  template <class _Ep, random_access_range _Range, weakly_incrementable _OutIter,
            class _RawPolicy = __remove_cvref_t<_Ep>, enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires sized_range<_Range> && indirectly_copyable<iterator_t<_Range>, _OutIter>
  _LIBCPP_HIDE_FROM_ABI copy_result<borrowed_iterator_t<_Range>, _OutIter>
  operator()(_Ep&& __exec, _Range&& __range, _OutIter __result) const {
    return (*this)(std::forward<_Ep>(__exec), ranges::begin(__range), ranges::end(__range), std::move(__result));
  }
#  endif
};

inline namespace __cpo {
inline constexpr auto copy = __copy{};
} // namespace __cpo
} // namespace ranges

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_STD_VER >= 20

_LIBCPP_POP_MACROS

#endif // _LIBCPP___ALGORITHM_RANGES_COPY_H
