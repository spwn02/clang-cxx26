//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#pragma GCC system_header

#ifndef _LIBCPP___NUMERIC_PSTL_FREESTANDING_H
#define _LIBCPP___NUMERIC_PSTL_FREESTANDING_H

#include <__config>
#include <__functional/operations.h>
#include <__iterator/cpp17_iterator_concepts.h>
#include <__iterator/iterator_traits.h>
#include <__type_traits/enable_if.h>
#include <__type_traits/is_execution_policy.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/pair.h>

#if _LIBCPP_STD_VER >= 17 && defined(_LIBCPP_FREESTANDING)
_LIBCPP_BEGIN_NAMESPACE_STD
template <class _ExecutionPolicy,
          class _ForwardIterator,
          class _Tp,
          class _BinaryOperation,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI _Tp reduce(
    _ExecutionPolicy&& __policy, _ForwardIterator __first, _ForwardIterator __last, _Tp __init, _BinaryOperation __op) = delete;

template <class _ExecutionPolicy,
          class _ForwardIterator,
          class _Tp,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI _Tp
reduce(_ExecutionPolicy&& __policy, _ForwardIterator __first, _ForwardIterator __last, _Tp __init) = delete;

template <class _ExecutionPolicy,
          class _ForwardIterator,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI __iterator_value_type<_ForwardIterator>
reduce(_ExecutionPolicy&& __policy, _ForwardIterator __first, _ForwardIterator __last) = delete;

template <class _ExecutionPolicy,
          class _ForwardIterator1,
          class _ForwardIterator2,
          class _Tp,
          class _BinaryOperation1,
          class _BinaryOperation2,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI _Tp transform_reduce(
    _ExecutionPolicy&& __policy,
    _ForwardIterator1 __first1,
    _ForwardIterator1 __last1,
    _ForwardIterator2 __first2,
    _Tp __init,
    _BinaryOperation1 __reduce,
    _BinaryOperation2 __transform) = delete;

template <class _ExecutionPolicy,
          class _ForwardIterator1,
          class _ForwardIterator2,
          class _Tp,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI _Tp transform_reduce(
    _ExecutionPolicy&& __policy,
    _ForwardIterator1 __first1,
    _ForwardIterator1 __last1,
    _ForwardIterator2 __first2,
    _Tp __init) = delete;

template <class _ExecutionPolicy,
          class _ForwardIterator,
          class _Tp,
          class _BinaryOperation,
          class _UnaryOperation,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI _Tp transform_reduce(
    _ExecutionPolicy&& __policy,
    _ForwardIterator __first,
    _ForwardIterator __last,
    _Tp __init,
    _BinaryOperation __reduce,
    _UnaryOperation __transform) = delete;

template <class _ExecutionPolicy,
          class _ForwardIterator,
          class _ForwardOutIterator,
          class _BinaryOperation,
          class _Tp,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI _ForwardOutIterator inclusive_scan(
    _ExecutionPolicy&& __policy,
    _ForwardIterator __first,
    _ForwardIterator __last,
    _ForwardOutIterator __result,
    _BinaryOperation __op,
    _Tp __init) = delete;

template <class _ExecutionPolicy,
          class _ForwardIterator,
          class _ForwardOutIterator,
          class _BinaryOperation,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI _ForwardOutIterator inclusive_scan(
    _ExecutionPolicy&& __policy,
    _ForwardIterator __first,
    _ForwardIterator __last,
    _ForwardOutIterator __result,
    _BinaryOperation __op) = delete;

template <class _ExecutionPolicy,
          class _ForwardIterator,
          class _ForwardOutIterator,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI _ForwardOutIterator inclusive_scan(
    _ExecutionPolicy&& __policy, _ForwardIterator __first, _ForwardIterator __last, _ForwardOutIterator __result) = delete;

template <class _ExecutionPolicy,
          class _ForwardIterator,
          class _ForwardOutIterator,
          class _Tp,
          class _BinaryOperation,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI _ForwardOutIterator exclusive_scan(
    _ExecutionPolicy&& __policy,
    _ForwardIterator __first,
    _ForwardIterator __last,
    _ForwardOutIterator __result,
    _Tp __init,
    _BinaryOperation __op) = delete;

template <class _ExecutionPolicy,
          class _ForwardIterator,
          class _ForwardOutIterator,
          class _Tp,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI _ForwardOutIterator exclusive_scan(
    _ExecutionPolicy&& __policy,
    _ForwardIterator __first,
    _ForwardIterator __last,
    _ForwardOutIterator __result,
    _Tp __init) = delete;

template <class _ExecutionPolicy,
          class _ForwardIterator,
          class _ForwardOutIterator,
          class _BinaryOperation,
          class _UnaryOperation,
          class _Tp,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI _ForwardOutIterator transform_inclusive_scan(
    _ExecutionPolicy&& __policy,
    _ForwardIterator __first,
    _ForwardIterator __last,
    _ForwardOutIterator __result,
    _BinaryOperation __bop,
    _UnaryOperation __uop,
    _Tp __init) = delete;

template <class _ExecutionPolicy,
          class _ForwardIterator,
          class _ForwardOutIterator,
          class _BinaryOperation,
          class _UnaryOperation,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI _ForwardOutIterator transform_inclusive_scan(
    _ExecutionPolicy&& __policy,
    _ForwardIterator __first,
    _ForwardIterator __last,
    _ForwardOutIterator __result,
    _BinaryOperation __bop,
    _UnaryOperation __uop) = delete;

template <class _ExecutionPolicy,
          class _ForwardIterator,
          class _ForwardOutIterator,
          class _Tp,
          class _BinaryOperation,
          class _UnaryOperation,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI _ForwardOutIterator transform_exclusive_scan(
    _ExecutionPolicy&& __policy,
    _ForwardIterator __first,
    _ForwardIterator __last,
    _ForwardOutIterator __result,
    _Tp __init,
    _BinaryOperation __bop,
    _UnaryOperation __uop) = delete;

template <class _ExecutionPolicy,
          class _ForwardIterator,
          class _ForwardOutIterator,
          class _BinaryOperation,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI _ForwardOutIterator adjacent_difference(
    _ExecutionPolicy&& __policy,
    _ForwardIterator __first,
    _ForwardIterator __last,
    _ForwardOutIterator __result,
    _BinaryOperation __op) = delete;

template <class _ExecutionPolicy,
          class _ForwardIterator,
          class _ForwardOutIterator,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI _ForwardOutIterator adjacent_difference(
    _ExecutionPolicy&& __policy, _ForwardIterator __first, _ForwardIterator __last, _ForwardOutIterator __result) = delete;
_LIBCPP_END_NAMESPACE_STD
#endif

#endif // _LIBCPP___NUMERIC_PSTL_FREESTANDING_H
