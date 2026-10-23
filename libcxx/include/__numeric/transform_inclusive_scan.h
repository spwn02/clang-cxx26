// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___NUMERIC_TRANSFORM_INCLUSIVE_SCAN_H
#define _LIBCPP___NUMERIC_TRANSFORM_INCLUSIVE_SCAN_H

#include <__config>
#include <__iterator/iterator_traits.h>
#include <__type_traits/invoke.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 17

template <class _InputIterator, class _OutputIterator, class _Tp, class _BinaryOp, class _UnaryOp>
_LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX20 _OutputIterator __transform_inclusive_scan_impl(
    _InputIterator __first, _InputIterator __last, _OutputIterator __result, _BinaryOp __b, _UnaryOp __u, _Tp __init) {
  for (; __first != __last; ++__first, (void)++__result) {
    __init    = __b(__init, __u(*__first));
    *__result = __init;
  }

  return __result;
}

template <class _InputIterator, class _OutputIterator, class _Tp, class _BinaryOp, class _UnaryOp>
_LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX20 _OutputIterator transform_inclusive_scan(
    _InputIterator __first, _InputIterator __last, _OutputIterator __result, _BinaryOp __b, _UnaryOp __u, _Tp __init) {
#if _LIBCPP_STD_VER >= 20
  using _TransformResult = __invoke_result_t<_UnaryOp&, decltype(*__first)>;
  static_assert(is_invocable_r_v<_Tp, _BinaryOp&, _Tp&, _Tp&> &&
                    is_invocable_r_v<_Tp, _BinaryOp&, _Tp&, _TransformResult> &&
                    is_invocable_r_v<_Tp, _BinaryOp&, _TransformResult, _TransformResult>,
                "transform_inclusive_scan requires all three binary_op argument combinations to produce T");
#endif
  return std::__transform_inclusive_scan_impl(__first, __last, __result, __b, __u, __init);
}

template <class _InputIterator, class _OutputIterator, class _BinaryOp, class _UnaryOp>
_LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX20 _OutputIterator transform_inclusive_scan(
    _InputIterator __first, _InputIterator __last, _OutputIterator __result, _BinaryOp __b, _UnaryOp __u) {
#if _LIBCPP_STD_VER >= 20
  using _TransformResult = __invoke_result_t<_UnaryOp&, decltype(*__first)>;
  using _ValueType       = typename iterator_traits<_InputIterator>::value_type;
  static_assert(is_invocable_r_v<_ValueType, _BinaryOp&, _TransformResult, _TransformResult>,
                "transform_inclusive_scan requires binary_op(unary_op(*first), unary_op(*first)) to produce the input value type");
#endif
  if (__first != __last) {
    typename iterator_traits<_InputIterator>::value_type __init = __u(*__first);
    *__result++                                                 = __init;
    if (++__first != __last)
      return std::__transform_inclusive_scan_impl(__first, __last, __result, __b, __u, __init);
  }

  return __result;
}

#endif // _LIBCPP_STD_VER >= 17

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___NUMERIC_TRANSFORM_INCLUSIVE_SCAN_H
