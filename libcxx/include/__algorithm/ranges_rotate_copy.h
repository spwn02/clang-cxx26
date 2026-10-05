//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___ALGORITHM_RANGES_ROTATE_COPY_H
#define _LIBCPP___ALGORITHM_RANGES_ROTATE_COPY_H

#include <__algorithm/in_in_out_result.h>
#include <__algorithm/in_out_result.h>
#include <__algorithm/pstl.h>
#include <__algorithm/ranges_copy.h>
#include <__config>
#include <__iterator/concepts.h>
#include <__pstl/ranges_bounded.h>
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

template <class _InIter, class _OutIter>
using rotate_copy_result = in_out_result<_InIter, _OutIter>;

// [algorithm.syn]: the execution-policy overloads return in_in_out_result<I, I, O>: where the input was consumed up to,
// where the reversed/rotated part starts, and the end of the written output.
template <class _InIter, class _OutIter>
using rotate_copy_truncated_result = in_in_out_result<_InIter, _InIter, _OutIter>;

struct __rotate_copy {
  template <forward_iterator _InIter, sentinel_for<_InIter> _Sent, weakly_incrementable _OutIter>
    requires indirectly_copyable<_InIter, _OutIter>
  _LIBCPP_HIDE_FROM_ABI constexpr rotate_copy_result<_InIter, _OutIter>
  operator()(_InIter __first, _InIter __middle, _Sent __last, _OutIter __result) const {
    auto __res1 = ranges::copy(__middle, __last, std::move(__result));
    auto __res2 = ranges::copy(__first, __middle, std::move(__res1.out));
    return {std::move(__res1.in), std::move(__res2.out)};
  }

  template <forward_range _Range, weakly_incrementable _OutIter>
    requires indirectly_copyable<iterator_t<_Range>, _OutIter>
  _LIBCPP_HIDE_FROM_ABI constexpr rotate_copy_result<borrowed_iterator_t<_Range>, _OutIter>
  operator()(_Range&& __range, iterator_t<_Range> __middle, _OutIter __result) const {
    return (*this)(ranges::begin(__range), std::move(__middle), ranges::end(__range), std::move(__result));
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
  _LIBCPP_HIDE_FROM_ABI rotate_copy_truncated_result<_InIter, _OutIter> operator()(
      _Ep&& __exec, _InIter __first, _InIter __middle, _Sent __last, _OutIter __result, _OutSent __result_last) const {
    using _Implementation = __pstl::__dispatch<__pstl::__ranges_bounded_rotate_copy, __pstl::__current_configuration, _RawPolicy>;
    return __pstl::__handle_exception<_Implementation>(std::forward<_Ep>(__exec), std::move(__first), std::move(__middle), std::move(__last), std::move(__result), std::move(__result_last));
  }

  template <class _Ep,
            random_access_range _Range,
            random_access_range _OutRange,
            class _RawPolicy = __remove_cvref_t<_Ep>,
            enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires sized_range<_Range> && sized_range<_OutRange> &&
             indirectly_copyable<iterator_t<_Range>, iterator_t<_OutRange>>
  _LIBCPP_HIDE_FROM_ABI rotate_copy_truncated_result<borrowed_iterator_t<_Range>, borrowed_iterator_t<_OutRange>>
  operator()(_Ep&& __exec, _Range&& __range, iterator_t<_Range> __middle, _OutRange&& __result_range) const {
    return (*this)(
        std::forward<_Ep>(__exec),
        ranges::begin(__range),
        std::move(__middle),
        ranges::begin(__range) + ranges::distance(__range),
        ranges::begin(__result_range),
        ranges::begin(__result_range) + ranges::distance(__result_range));
  }
#  endif
};

inline namespace __cpo {
inline constexpr auto rotate_copy = __rotate_copy{};
} // namespace __cpo
} // namespace ranges

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_STD_VER >= 20

_LIBCPP_POP_MACROS

#endif // _LIBCPP___ALGORITHM_RANGES_ROTATE_COPY_H
