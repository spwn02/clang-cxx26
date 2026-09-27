//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___PSTL_RANGES_BOUNDED_H
#define _LIBCPP___PSTL_RANGES_BOUNDED_H

#include <__algorithm/in_in_out_result.h>
#include <__algorithm/in_out_result.h>
#include <__config>
#include <__functional/invoke.h>
#include <__pstl/backend_fwd.h>
#include <optional>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

#if _LIBCPP_STD_VER >= 26

_LIBCPP_BEGIN_NAMESPACE_STD
namespace __pstl {

// A bounded set operation may pass over input elements which produce no output
// even after the output range is full. Each backend stops at the first element
// it would have to write beyond the output sentinel.
template <class _ExecutionPolicy>
struct __ranges_bounded_set_union<__default_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _Iter1, class _Sent1, class _Iter2, class _Sent2,
            class _OutIter, class _OutSent, class _Comp, class _Proj1, class _Proj2>
  _LIBCPP_HIDE_FROM_ABI optional<ranges::in_in_out_result<_Iter1, _Iter2, _OutIter>> operator()(
      _Policy&&, _Iter1 __first1, _Sent1 __last1, _Iter2 __first2, _Sent2 __last2,
      _OutIter __result, _OutSent __result_last, _Comp __comp, _Proj1 __proj1, _Proj2 __proj2) const noexcept {
    while (__first1 != __last1 && __first2 != __last2) {
      if (__result == __result_last)
        return ranges::in_in_out_result<_Iter1, _Iter2, _OutIter>{__first1, __first2, __result};
      if (std::invoke(__comp, std::invoke(__proj2, *__first2), std::invoke(__proj1, *__first1))) {
        *__result = *__first2;
        ++__first2;
      } else {
        *__result = *__first1;
        if (!std::invoke(__comp, std::invoke(__proj1, *__first1), std::invoke(__proj2, *__first2)))
          ++__first2;
        ++__first1;
      }
      ++__result;
    }
    while (__first1 != __last1) {
      if (__result == __result_last)
        break;
      *__result = *__first1;
      ++__first1;
      ++__result;
    }
    while (__first2 != __last2) {
      if (__result == __result_last)
        break;
      *__result = *__first2;
      ++__first2;
      ++__result;
    }
    return ranges::in_in_out_result<_Iter1, _Iter2, _OutIter>{__first1, __first2, __result};
  }
};

template <class _ExecutionPolicy>
struct __ranges_bounded_set_intersection<__default_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _Iter1, class _Sent1, class _Iter2, class _Sent2,
            class _OutIter, class _OutSent, class _Comp, class _Proj1, class _Proj2>
  _LIBCPP_HIDE_FROM_ABI optional<ranges::in_in_out_result<_Iter1, _Iter2, _OutIter>> operator()(
      _Policy&&, _Iter1 __first1, _Sent1 __last1, _Iter2 __first2, _Sent2 __last2,
      _OutIter __result, _OutSent __result_last, _Comp __comp, _Proj1 __proj1, _Proj2 __proj2) const noexcept {
    while (__first1 != __last1 && __first2 != __last2) {
      if (std::invoke(__comp, std::invoke(__proj1, *__first1), std::invoke(__proj2, *__first2))) {
        ++__first1;
      } else if (std::invoke(__comp, std::invoke(__proj2, *__first2), std::invoke(__proj1, *__first1))) {
        ++__first2;
      } else {
        if (__result == __result_last)
          break;
        *__result = *__first1;
        ++__first1;
        ++__first2;
        ++__result;
      }
    }
    return ranges::in_in_out_result<_Iter1, _Iter2, _OutIter>{__first1, __first2, __result};
  }
};

template <class _ExecutionPolicy>
struct __ranges_bounded_set_difference<__default_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _Iter1, class _Sent1, class _Iter2, class _Sent2,
            class _OutIter, class _OutSent, class _Comp, class _Proj1, class _Proj2>
  _LIBCPP_HIDE_FROM_ABI optional<ranges::in_out_result<_Iter1, _OutIter>> operator()(
      _Policy&&, _Iter1 __first1, _Sent1 __last1, _Iter2 __first2, _Sent2 __last2,
      _OutIter __result, _OutSent __result_last, _Comp __comp, _Proj1 __proj1, _Proj2 __proj2) const noexcept {
    while (__first1 != __last1 && __first2 != __last2) {
      if (std::invoke(__comp, std::invoke(__proj1, *__first1), std::invoke(__proj2, *__first2))) {
        if (__result == __result_last)
          return ranges::in_out_result<_Iter1, _OutIter>{__first1, __result};
        *__result = *__first1;
        ++__first1;
        ++__result;
      } else if (std::invoke(__comp, std::invoke(__proj2, *__first2), std::invoke(__proj1, *__first1))) {
        ++__first2;
      } else {
        ++__first1;
        ++__first2;
      }
    }
    while (__first1 != __last1 && __result != __result_last) {
      *__result = *__first1;
      ++__first1;
      ++__result;
    }
    return ranges::in_out_result<_Iter1, _OutIter>{__first1, __result};
  }
};

template <class _ExecutionPolicy>
struct __ranges_bounded_set_symmetric_difference<__default_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _Iter1, class _Sent1, class _Iter2, class _Sent2,
            class _OutIter, class _OutSent, class _Comp, class _Proj1, class _Proj2>
  _LIBCPP_HIDE_FROM_ABI optional<ranges::in_in_out_result<_Iter1, _Iter2, _OutIter>> operator()(
      _Policy&&, _Iter1 __first1, _Sent1 __last1, _Iter2 __first2, _Sent2 __last2,
      _OutIter __result, _OutSent __result_last, _Comp __comp, _Proj1 __proj1, _Proj2 __proj2) const noexcept {
    while (__first1 != __last1 && __first2 != __last2) {
      if (std::invoke(__comp, std::invoke(__proj1, *__first1), std::invoke(__proj2, *__first2))) {
        if (__result == __result_last)
          return ranges::in_in_out_result<_Iter1, _Iter2, _OutIter>{__first1, __first2, __result};
        *__result = *__first1;
        ++__first1;
        ++__result;
      } else if (std::invoke(__comp, std::invoke(__proj2, *__first2), std::invoke(__proj1, *__first1))) {
        if (__result == __result_last)
          return ranges::in_in_out_result<_Iter1, _Iter2, _OutIter>{__first1, __first2, __result};
        *__result = *__first2;
        ++__first2;
        ++__result;
      } else {
        ++__first1;
        ++__first2;
      }
    }
    while (__first1 != __last1 && __result != __result_last) {
      *__result = *__first1;
      ++__first1;
      ++__result;
    }
    while (__first2 != __last2 && __result != __result_last) {
      *__result = *__first2;
      ++__first2;
      ++__result;
    }
    return ranges::in_in_out_result<_Iter1, _Iter2, _OutIter>{__first1, __first2, __result};
  }
};

template <class _ExecutionPolicy>
struct __ranges_bounded_unique_copy<__default_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _Iter, class _Sent, class _OutIter, class _OutSent, class _Comp, class _Proj>
  _LIBCPP_HIDE_FROM_ABI optional<ranges::in_out_result<_Iter, _OutIter>> operator()(
      _Policy&&, _Iter __first, _Sent __last, _OutIter __result, _OutSent __result_last,
      _Comp __comp, _Proj __proj) const noexcept {
    if (__first == __last || __result == __result_last)
      return ranges::in_out_result<_Iter, _OutIter>{__first, __result};
    *__result = *__first;
    ++__result;
    _Iter __previous = __first;
    ++__first;
    while (__first != __last) {
      if (!std::invoke(__comp, std::invoke(__proj, *__first), std::invoke(__proj, *__previous))) {
        if (__result == __result_last)
          break;
        *__result = *__first;
        ++__result;
      }
      __previous = __first;
      ++__first;
    }
    return ranges::in_out_result<_Iter, _OutIter>{__first, __result};
  }
};

} // namespace __pstl
_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_POP_MACROS

#endif // _LIBCPP___PSTL_RANGES_BOUNDED_H
