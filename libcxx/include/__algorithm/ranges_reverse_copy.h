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

// [alg.reverse.copy], per P3709R2: the execution-policy overload reports where copying
// ceased. in1 is always `last` (the end of the input range: reverse_copy always knows this
// upfront, unlike rotate_copy's two-subrange split); in2 is the point in the input the
// algorithm reached before ceasing (reverse_copy iterates from `last` down to `first`, so on
// full completion in2 == first).
template <class _InIter1, class _InIter2, class _OutIter>
using reverse_copy_truncated_result = in_in_out_result<_InIter1, _InIter2, _OutIter>;

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

#  if _LIBCPP_HAS_EXPERIMENTAL_PSTL
  // This fork's PSTL backend (__pstl::__handle_exception) is all-or-nothing: it either
  // completes the whole operation or throws, with no partial/truncated outcome ever
  // observable by the caller. So on (the only possible) success, the entire input has been
  // consumed: in1 == last, in2 == first. This is a correct representation given this
  // backend's actual capabilities, not an approximation -- a backend that could genuinely
  // stop partway would need to plumb real progress information through, which this one
  // does not have.
  template <class _Ep,
            bidirectional_iterator _InIter,
            sized_sentinel_for<_InIter> _Sent,
            weakly_incrementable _OutIter,
            class _RawPolicy                                    = __remove_cvref_t<_Ep>,
            enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires indirectly_copyable<_InIter, _OutIter>
  _LIBCPP_HIDE_FROM_ABI reverse_copy_truncated_result<_Sent, _InIter, _OutIter>
  operator()(_Ep&& __exec, _InIter __first, _Sent __last, _OutIter __result) const {
    _InIter __end = __first + (__last - __first);
    _OutIter __out = std::reverse_copy(std::forward<_Ep>(__exec), __first, __end, std::move(__result));
    return {__last, std::move(__first), std::move(__out)};
  }

  template <class _Ep,
            bidirectional_range _Range,
            weakly_incrementable _OutIter,
            class _RawPolicy                                    = __remove_cvref_t<_Ep>,
            enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires indirectly_copyable<iterator_t<_Range>, _OutIter> && sized_range<_Range>
  _LIBCPP_HIDE_FROM_ABI reverse_copy_truncated_result<sentinel_t<_Range>, iterator_t<_Range>, _OutIter>
  operator()(_Ep&& __exec, _Range&& __range, _OutIter __result) const {
    return (*this)(std::forward<_Ep>(__exec), ranges::begin(__range), ranges::end(__range), std::move(__result));
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
