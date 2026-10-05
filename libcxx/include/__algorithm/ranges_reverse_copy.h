//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___ALGORITHM_RANGES_REVERSE_COPY_H
#define _LIBCPP___ALGORITHM_RANGES_REVERSE_COPY_H

#include <__algorithm/in_in_out_result.h>
#include <__algorithm/in_out_result.h>
#include <__algorithm/pstl.h>
#include <__algorithm/ranges_copy.h>
#include <__config>
#include <__iterator/concepts.h>
#include <__iterator/next.h>
#include <__iterator/reverse_iterator.h>
#include <__pstl/ranges_bounded.h>
#include <__ranges/access.h>
#include <__ranges/concepts.h>
#include <__ranges/dangling.h>
#include <__ranges/subrange.h>
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
using reverse_copy_result = in_out_result<_InIter, _OutIter>;

// [algorithm.syn]: the execution-policy overloads return in_in_out_result<I, I, O>: where the input was consumed up to,
// where the reversed/rotated part starts, and the end of the written output.
template <class _InIter, class _OutIter>
using reverse_copy_truncated_result = in_in_out_result<_InIter, _InIter, _OutIter>;

struct __reverse_copy {
  template <bidirectional_iterator _InIter, sentinel_for<_InIter> _Sent, weakly_incrementable _OutIter>
    requires indirectly_copyable<_InIter, _OutIter>
  _LIBCPP_HIDE_FROM_ABI constexpr reverse_copy_result<_InIter, _OutIter>
  operator()(_InIter __first, _Sent __last, _OutIter __result) const {
    return (*this)(subrange(std::move(__first), std::move(__last)), std::move(__result));
  }

  template <bidirectional_range _Range, weakly_incrementable _OutIter>
    requires indirectly_copyable<iterator_t<_Range>, _OutIter>
  _LIBCPP_HIDE_FROM_ABI constexpr reverse_copy_result<borrowed_iterator_t<_Range>, _OutIter>
  operator()(_Range&& __range, _OutIter __result) const {
    auto __ret = ranges::copy(std::__reverse_range(__range), std::move(__result));
    return {ranges::next(ranges::begin(__range), ranges::end(__range)), std::move(__ret.out)};
  }

#  if _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_EXPERIMENTAL_PSTL && !defined(_LIBCPP_FREESTANDING)
  template <class _Ep,
            random_access_iterator _InIter,
            sized_sentinel_for<_InIter> _Sent,
            random_access_iterator _OutIter,
            sized_sentinel_for<_OutIter> _OutSent,
            class _RawPolicy = __remove_cvref_t<_Ep>,
            enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires indirectly_copyable<_InIter, _OutIter>
  _LIBCPP_HIDE_FROM_ABI reverse_copy_truncated_result<_InIter, _OutIter>
  operator()(_Ep&& __exec, _InIter __first, _Sent __last, _OutIter __result, _OutSent __result_last) const {
    using _Implementation = __pstl::__dispatch<__pstl::__ranges_bounded_reverse_copy, __pstl::__current_configuration, _RawPolicy>;
    return __pstl::__handle_exception<_Implementation>(std::forward<_Ep>(__exec), std::move(__first), std::move(__last), std::move(__result), std::move(__result_last));
  }

  template <class _Ep,
            random_access_range _Range,
            random_access_range _OutRange,
            class _RawPolicy = __remove_cvref_t<_Ep>,
            enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires sized_range<_Range> && sized_range<_OutRange> &&
             indirectly_copyable<iterator_t<_Range>, iterator_t<_OutRange>>
  _LIBCPP_HIDE_FROM_ABI reverse_copy_truncated_result<borrowed_iterator_t<_Range>, borrowed_iterator_t<_OutRange>>
  operator()(_Ep&& __exec, _Range&& __range, _OutRange&& __result_range) const {
    return (*this)(
        std::forward<_Ep>(__exec),
        ranges::begin(__range),
        ranges::begin(__range) + ranges::size(__range),
        ranges::begin(__result_range),
        ranges::begin(__result_range) + ranges::size(__result_range));
  }
#  endif
};

inline namespace __cpo {
inline constexpr auto reverse_copy = __reverse_copy{};
} // namespace __cpo
} // namespace ranges

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_STD_VER >= 20

_LIBCPP_POP_MACROS

#endif // _LIBCPP___ALGORITHM_RANGES_REVERSE_COPY_H
