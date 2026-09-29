//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#pragma GCC system_header

#ifndef _LIBCPP___MEMORY_PSTL_FREESTANDING_H
#define _LIBCPP___MEMORY_PSTL_FREESTANDING_H

#include <__config>
#include <__type_traits/enable_if.h>
#include <__type_traits/is_execution_policy.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/pair.h>

#if _LIBCPP_STD_VER >= 17 && defined(_LIBCPP_FREESTANDING)
_LIBCPP_BEGIN_NAMESPACE_STD

template <class _ExecutionPolicy, class _ForwardIterator,
          enable_if_t<is_execution_policy_v<__remove_cvref_t<_ExecutionPolicy>>, int> = 0>
void uninitialized_default_construct(_ExecutionPolicy&&, _ForwardIterator, _ForwardIterator) = delete;

template <class _ExecutionPolicy, class _ForwardIterator, class _Size,
          enable_if_t<is_execution_policy_v<__remove_cvref_t<_ExecutionPolicy>>, int> = 0>
_ForwardIterator uninitialized_default_construct_n(_ExecutionPolicy&&, _ForwardIterator, _Size) = delete;

template <class _ExecutionPolicy, class _ForwardIterator,
          enable_if_t<is_execution_policy_v<__remove_cvref_t<_ExecutionPolicy>>, int> = 0>
void uninitialized_value_construct(_ExecutionPolicy&&, _ForwardIterator, _ForwardIterator) = delete;

template <class _ExecutionPolicy, class _ForwardIterator, class _Size,
          enable_if_t<is_execution_policy_v<__remove_cvref_t<_ExecutionPolicy>>, int> = 0>
_ForwardIterator uninitialized_value_construct_n(_ExecutionPolicy&&, _ForwardIterator, _Size) = delete;

template <class _ExecutionPolicy, class _InputIterator, class _ForwardIterator,
          enable_if_t<is_execution_policy_v<__remove_cvref_t<_ExecutionPolicy>>, int> = 0>
_ForwardIterator uninitialized_copy(_ExecutionPolicy&&, _InputIterator, _InputIterator, _ForwardIterator) = delete;

template <class _ExecutionPolicy, class _InputIterator, class _Size, class _ForwardIterator,
          enable_if_t<is_execution_policy_v<__remove_cvref_t<_ExecutionPolicy>>, int> = 0>
pair<_InputIterator, _ForwardIterator>
uninitialized_copy_n(_ExecutionPolicy&&, _InputIterator, _Size, _ForwardIterator) = delete;

template <class _ExecutionPolicy, class _InputIterator, class _ForwardIterator,
          enable_if_t<is_execution_policy_v<__remove_cvref_t<_ExecutionPolicy>>, int> = 0>
_ForwardIterator uninitialized_move(_ExecutionPolicy&&, _InputIterator, _InputIterator, _ForwardIterator) = delete;

template <class _ExecutionPolicy, class _InputIterator, class _Size, class _ForwardIterator,
          enable_if_t<is_execution_policy_v<__remove_cvref_t<_ExecutionPolicy>>, int> = 0>
pair<_InputIterator, _ForwardIterator>
uninitialized_move_n(_ExecutionPolicy&&, _InputIterator, _Size, _ForwardIterator) = delete;

template <class _ExecutionPolicy, class _ForwardIterator, class _Tp,
          enable_if_t<is_execution_policy_v<__remove_cvref_t<_ExecutionPolicy>>, int> = 0>
void uninitialized_fill(_ExecutionPolicy&&, _ForwardIterator, _ForwardIterator, const _Tp&) = delete;

template <class _ExecutionPolicy, class _ForwardIterator, class _Size, class _Tp,
          enable_if_t<is_execution_policy_v<__remove_cvref_t<_ExecutionPolicy>>, int> = 0>
_ForwardIterator uninitialized_fill_n(_ExecutionPolicy&&, _ForwardIterator, _Size, const _Tp&) = delete;

template <class _ExecutionPolicy, class _ForwardIterator,
          enable_if_t<is_execution_policy_v<__remove_cvref_t<_ExecutionPolicy>>, int> = 0>
void destroy(_ExecutionPolicy&&, _ForwardIterator, _ForwardIterator) = delete;

template <class _ExecutionPolicy, class _ForwardIterator, class _Size,
          enable_if_t<is_execution_policy_v<__remove_cvref_t<_ExecutionPolicy>>, int> = 0>
_ForwardIterator destroy_n(_ExecutionPolicy&&, _ForwardIterator, _Size) = delete;

_LIBCPP_END_NAMESPACE_STD
#endif

#endif // _LIBCPP___MEMORY_PSTL_FREESTANDING_H
