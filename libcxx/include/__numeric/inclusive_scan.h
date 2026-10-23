// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___NUMERIC_INCLUSIVE_SCAN_H
#define _LIBCPP___NUMERIC_INCLUSIVE_SCAN_H

#include <__config>
#include <__functional/operations.h>
#include <__iterator/iterator_traits.h>
#include <__type_traits/invoke.h>
#include <__utility/move.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 17

template <class _InputIterator, class _OutputIterator, class _Tp, class _BinaryOp>
_LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX20 _OutputIterator
__inclusive_scan_impl(_InputIterator __first, _InputIterator __last, _OutputIterator __result, _BinaryOp __b, _Tp __init) {
  for (; __first != __last; ++__first, (void)++__result) {
    __init    = __b(__init, *__first);
    *__result = __init;
  }
  return __result;
}

template <class _InputIterator, class _OutputIterator, class _Tp, class _BinaryOp>
_LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX20 _OutputIterator
inclusive_scan(_InputIterator __first, _InputIterator __last, _OutputIterator __result, _BinaryOp __b, _Tp __init) {
#if _LIBCPP_STD_VER >= 20
  using _InputReference = decltype(*__first);
  static_assert(is_invocable_r_v<_Tp, _BinaryOp&, _Tp&, _Tp&> &&
                    is_invocable_r_v<_Tp, _BinaryOp&, _Tp&, _InputReference> &&
                    is_invocable_r_v<_Tp, _BinaryOp&, _InputReference, _InputReference>,
                "inclusive_scan requires all three binary_op argument combinations to produce T");
#endif
  return std::__inclusive_scan_impl(__first, __last, __result, __b, __init);
}

template <class _InputIterator, class _OutputIterator, class _BinaryOp>
_LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX20 _OutputIterator
inclusive_scan(_InputIterator __first, _InputIterator __last, _OutputIterator __result, _BinaryOp __b) {
#if _LIBCPP_STD_VER >= 20
  using _InputReference = decltype(*__first);
  using _ValueType      = typename iterator_traits<_InputIterator>::value_type;
  static_assert(is_invocable_r_v<_ValueType, _BinaryOp&, _InputReference, _InputReference>,
                "inclusive_scan requires binary_op(*first, *first) to produce the input value type");
#endif
  if (__first != __last) {
    typename iterator_traits<_InputIterator>::value_type __init = *__first;
    *__result++                                                 = __init;
    if (++__first != __last)
      return std::__inclusive_scan_impl(__first, __last, __result, __b, __init);
  }

  return __result;
}

template <class _InputIterator, class _OutputIterator>
_LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX20 _OutputIterator
inclusive_scan(_InputIterator __first, _InputIterator __last, _OutputIterator __result) {
  return std::inclusive_scan(__first, __last, __result, std::plus<>());
}

#endif // _LIBCPP_STD_VER >= 17

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___NUMERIC_INCLUSIVE_SCAN_H
