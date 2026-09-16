//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___ALGORITHM_RANGES_FIND_LAST_H
#define _LIBCPP___ALGORITHM_RANGES_FIND_LAST_H

#include <__algorithm/pstl.h>
#include <__algorithm/ranges_find.h>
#include <__algorithm/ranges_find_if.h>
#include <__algorithm/ranges_find_if_not.h>
#include <__config>
#include <__functional/identity.h>
#include <__functional/invoke.h>
#include <__functional/ranges_operations.h>
#include <__iterator/concepts.h>
#include <__iterator/indirectly_comparable.h>
#include <__iterator/next.h>
#include <__iterator/prev.h>
#include <__iterator/projected.h>
#include <__iterator/reverse_iterator.h>
#include <__ranges/access.h>
#include <__ranges/concepts.h>
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

#if _LIBCPP_STD_VER >= 23

_LIBCPP_BEGIN_NAMESPACE_STD

namespace ranges {

template <class _Iter, class _Sent, class _Pred, class _Proj>
_LIBCPP_HIDE_FROM_ABI constexpr subrange<_Iter>
__find_last_impl(_Iter __first, _Sent __last, _Pred __pred, _Proj& __proj) {
  if (__first == __last) {
    return subrange<_Iter>(__first, __first);
  }

  if constexpr (bidirectional_iterator<_Iter>) {
    auto __last_it = ranges::next(__first, __last);
    for (auto __it = ranges::prev(__last_it); __it != __first; --__it) {
      if (__pred(std::invoke(__proj, *__it))) {
        return subrange<_Iter>(std::move(__it), std::move(__last_it));
      }
    }
    if (__pred(std::invoke(__proj, *__first))) {
      return subrange<_Iter>(std::move(__first), std::move(__last_it));
    }
    return subrange<_Iter>(__last_it, __last_it);
  } else {
    bool __found = false;
    _Iter __found_it;
    for (; __first != __last; ++__first) {
      if (__pred(std::invoke(__proj, *__first))) {
        __found    = true;
        __found_it = __first;
      }
    }

    if (__found) {
      return subrange<_Iter>(std::move(__found_it), std::move(__first));
    } else {
      return subrange<_Iter>(__first, __first);
    }
  }
}

struct __find_last {
  template <class _Type>
  struct __op {
    const _Type& __value;
    template <class _Elem>
    _LIBCPP_HIDE_FROM_ABI constexpr decltype(auto) operator()(_Elem&& __elem) const {
      return std::forward<_Elem>(__elem) == __value;
    }
  };

  template <forward_iterator _Iter,
            sentinel_for<_Iter> _Sent,
            class _Proj = identity,
            class _Type
#  if _LIBCPP_STD_VER >= 26
            = projected_value_t<_Iter, _Proj>
#  endif
            >
    requires indirect_binary_predicate<ranges::equal_to, projected<_Iter, _Proj>, const _Type*>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr static subrange<_Iter>
  operator()(_Iter __first, _Sent __last, const _Type& __value, _Proj __proj = {}) {
    return ranges::__find_last_impl(std::move(__first), std::move(__last), __op<_Type>{__value}, __proj);
  }

  template <forward_range _Range,
            class _Proj = identity,
            class _Type
#  if _LIBCPP_STD_VER >= 26
            = projected_value_t<iterator_t<_Range>, _Proj>
#  endif
            >
    requires indirect_binary_predicate<ranges::equal_to, projected<iterator_t<_Range>, _Proj>, const _Type*>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr static borrowed_subrange_t<_Range>
  operator()(_Range&& __range, const _Type& __value, _Proj __proj = {}) {
    return ranges::__find_last_impl(ranges::begin(__range), ranges::end(__range), __op<_Type>{__value}, __proj);
  }
#  if _LIBCPP_HAS_EXPERIMENTAL_PSTL
  // Composes via the already-PSTL-backed ranges::find, scanning in reverse (find_last(first,
  // last, v) is find(reverse(last), reverse(first), v), mapped back to a forward position) --
  // this routes through the same __pstl::__handle_exception/backend-dispatch machinery
  // ranges::find already has, rather than a hand-rolled loop that would silently skip the
  // required parallel-algorithm exception semantics ([algorithms.parallel.exceptions]).
  template <class _Ep,
            random_access_iterator _Iter,
            sized_sentinel_for<_Iter> _Sent,
            class _Proj = identity,
            class _Type
#  if _LIBCPP_STD_VER >= 26
            = projected_value_t<_Iter, _Proj>
#  endif
            ,
            class _RawPolicy                                   = __remove_cvref_t<_Ep>,
            enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires indirect_binary_predicate<ranges::equal_to, projected<_Iter, _Proj>, const _Type*>
  _LIBCPP_HIDE_FROM_ABI subrange<_Iter>
  operator()(_Ep&& __exec, _Iter __first, _Sent __last, const _Type& __value, _Proj __proj = {}) const {
    _Iter __end = __first + (__last - __first);
    auto __rresult =
        ranges::find(std::forward<_Ep>(__exec), std::make_reverse_iterator(__end), std::make_reverse_iterator(__first),
                     __value, __proj);
    if (__rresult == std::make_reverse_iterator(__first))
      return {__end, __end};
    return {__rresult.base() - 1, __end};
  }

  template <class _Ep,
            random_access_range _Range,
            class _Proj = identity,
            class _Type
#  if _LIBCPP_STD_VER >= 26
            = projected_value_t<iterator_t<_Range>, _Proj>
#  endif
            ,
            class _RawPolicy                                   = __remove_cvref_t<_Ep>,
            enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires sized_range<_Range> && indirect_binary_predicate<ranges::equal_to, projected<iterator_t<_Range>, _Proj>, const _Type*>
  _LIBCPP_HIDE_FROM_ABI borrowed_subrange_t<_Range>
  operator()(_Ep&& __exec, _Range&& __r, const _Type& __value, _Proj __proj = {}) const {
    return (*this)(std::forward<_Ep>(__exec), ranges::begin(__r), ranges::end(__r), __value, std::move(__proj));
  }
#  endif
};

struct __find_last_if {
  template <class _Pred>
  struct __op {
    _Pred& __pred;
    template <class _Elem>
    _LIBCPP_HIDE_FROM_ABI constexpr decltype(auto) operator()(_Elem&& __elem) const {
      return std::invoke(__pred, std::forward<_Elem>(__elem));
    }
  };

  template <forward_iterator _Iter,
            sentinel_for<_Iter> _Sent,
            class _Proj = identity,
            indirect_unary_predicate<projected<_Iter, _Proj>> _Pred>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr static subrange<_Iter>
  operator()(_Iter __first, _Sent __last, _Pred __pred, _Proj __proj = {}) {
    return ranges::__find_last_impl(std::move(__first), std::move(__last), __op<_Pred>{__pred}, __proj);
  }

  template <forward_range _Range,
            class _Proj = identity,
            indirect_unary_predicate<projected<iterator_t<_Range>, _Proj>> _Pred>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr static borrowed_subrange_t<_Range>
  operator()(_Range&& __range, _Pred __pred, _Proj __proj = {}) {
    return ranges::__find_last_impl(ranges::begin(__range), ranges::end(__range), __op<_Pred>{__pred}, __proj);
  }
#  if _LIBCPP_HAS_EXPERIMENTAL_PSTL
  // See __find_last's own PSTL overload above for why this composes via ranges::find_if
  // (already PSTL-backed) on a reversed range, rather than a hand-rolled loop.
  template <class _Ep,
            random_access_iterator _Iter,
            sized_sentinel_for<_Iter> _Sent,
            class _Proj = identity,
            indirect_unary_predicate<projected<_Iter, _Proj>> _Pred,
            class _RawPolicy                                   = __remove_cvref_t<_Ep>,
            enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
  _LIBCPP_HIDE_FROM_ABI subrange<_Iter>
  operator()(_Ep&& __exec, _Iter __first, _Sent __last, _Pred __pred, _Proj __proj = {}) const {
    _Iter __end = __first + (__last - __first);
    auto __rresult = ranges::find_if(
        std::forward<_Ep>(__exec), std::make_reverse_iterator(__end), std::make_reverse_iterator(__first), __pred,
        __proj);
    if (__rresult == std::make_reverse_iterator(__first))
      return {__end, __end};
    return {__rresult.base() - 1, __end};
  }

  template <class _Ep,
            random_access_range _Range,
            class _Proj = identity,
            indirect_unary_predicate<projected<iterator_t<_Range>, _Proj>> _Pred,
            class _RawPolicy                                   = __remove_cvref_t<_Ep>,
            enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires sized_range<_Range>
  _LIBCPP_HIDE_FROM_ABI borrowed_subrange_t<_Range>
  operator()(_Ep&& __exec, _Range&& __r, _Pred __pred, _Proj __proj = {}) const {
    return (*this)(std::forward<_Ep>(__exec), ranges::begin(__r), ranges::end(__r), std::move(__pred), std::move(__proj));
  }
#  endif
};

struct __find_last_if_not {
  template <class _Pred>
  struct __op {
    _Pred& __pred;
    template <class _Elem>
    _LIBCPP_HIDE_FROM_ABI constexpr decltype(auto) operator()(_Elem&& __elem) const {
      return !std::invoke(__pred, std::forward<_Elem>(__elem));
    }
  };

  template <forward_iterator _Iter,
            sentinel_for<_Iter> _Sent,
            class _Proj = identity,
            indirect_unary_predicate<projected<_Iter, _Proj>> _Pred>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr static subrange<_Iter>
  operator()(_Iter __first, _Sent __last, _Pred __pred, _Proj __proj = {}) {
    return ranges::__find_last_impl(std::move(__first), std::move(__last), __op<_Pred>{__pred}, __proj);
  }

  template <forward_range _Range,
            class _Proj = identity,
            indirect_unary_predicate<projected<iterator_t<_Range>, _Proj>> _Pred>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr static borrowed_subrange_t<_Range>
  operator()(_Range&& __range, _Pred __pred, _Proj __proj = {}) {
    return ranges::__find_last_impl(ranges::begin(__range), ranges::end(__range), __op<_Pred>{__pred}, __proj);
  }
#  if _LIBCPP_HAS_EXPERIMENTAL_PSTL
  // See __find_last's own PSTL overload above for why this composes via ranges::find_if_not
  // (already PSTL-backed) on a reversed range, rather than a hand-rolled loop.
  template <class _Ep,
            random_access_iterator _Iter,
            sized_sentinel_for<_Iter> _Sent,
            class _Proj = identity,
            indirect_unary_predicate<projected<_Iter, _Proj>> _Pred,
            class _RawPolicy                                   = __remove_cvref_t<_Ep>,
            enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
  _LIBCPP_HIDE_FROM_ABI subrange<_Iter>
  operator()(_Ep&& __exec, _Iter __first, _Sent __last, _Pred __pred, _Proj __proj = {}) const {
    _Iter __end = __first + (__last - __first);
    auto __rresult = ranges::find_if_not(
        std::forward<_Ep>(__exec), std::make_reverse_iterator(__end), std::make_reverse_iterator(__first), __pred,
        __proj);
    if (__rresult == std::make_reverse_iterator(__first))
      return {__end, __end};
    return {__rresult.base() - 1, __end};
  }

  template <class _Ep,
            random_access_range _Range,
            class _Proj = identity,
            indirect_unary_predicate<projected<iterator_t<_Range>, _Proj>> _Pred,
            class _RawPolicy                                   = __remove_cvref_t<_Ep>,
            enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires sized_range<_Range>
  _LIBCPP_HIDE_FROM_ABI borrowed_subrange_t<_Range>
  operator()(_Ep&& __exec, _Range&& __r, _Pred __pred, _Proj __proj = {}) const {
    return (*this)(std::forward<_Ep>(__exec), ranges::begin(__r), ranges::end(__r), std::move(__pred), std::move(__proj));
  }
#  endif
};

inline namespace __cpo {
inline constexpr auto find_last        = __find_last{};
inline constexpr auto find_last_if     = __find_last_if{};
inline constexpr auto find_last_if_not = __find_last_if_not{};
} // namespace __cpo
} // namespace ranges

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_STD_VER >= 23

_LIBCPP_POP_MACROS

#endif // _LIBCPP___ALGORITHM_RANGES_FIND_LAST_H
