// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___MEMORY_PSTL_UNINITIALIZED_ALGORITHMS_H
#define _LIBCPP___MEMORY_PSTL_UNINITIALIZED_ALGORITHMS_H

#include <__pstl/backend_fwd.h>
#include <__pstl/dispatch.h>
#include <__pstl/handle_exception.h>
#include <__config>
#include <__iterator/distance.h>
#include <__iterator/iterator_traits.h>
#include <__memory/destroy.h>
#include <__memory/uninitialized_algorithms.h>
#include <__pstl/memory_algorithms.h>
#include <__type_traits/enable_if.h>
#include <__type_traits/is_execution_policy.h>
#include <__type_traits/remove_cvref.h>
#include <__type_traits/type_identity.h>
#include <__utility/forward.h>
#include <__utility/move.h>
#include <__utility/pair.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

// [memory.syn], [specialized.algorithms]: the overloads with an execution policy. Like the other parallel algorithms
// they are available with the experimental library; their semantics are those of the overloads without the policy
// ([algorithms.parallel.overloads]).
#if _LIBCPP_STD_VER >= 17 && _LIBCPP_HAS_EXPERIMENTAL_PSTL && !defined(_LIBCPP_FREESTANDING)

_LIBCPP_BEGIN_NAMESPACE_STD

// uninitialized_default_construct

template <class _ExecutionPolicy,
          class _NoThrowForwardIterator,
          class _RawPolicy                                  = __remove_cvref_t<_ExecutionPolicy>,
          __enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI void
uninitialized_default_construct(_ExecutionPolicy&& __exec, _NoThrowForwardIterator __first, _NoThrowForwardIterator __last) {
  using _ValueType      = typename iterator_traits<_NoThrowForwardIterator>::value_type;
  using _Implementation = __pstl::__dispatch<__pstl::__memory_default_construct_n, __pstl::__current_configuration, _RawPolicy>;
  auto __n              = std::distance(__first, __last);
  (void)__pstl::__handle_exception<_Implementation>(
      std::forward<_ExecutionPolicy>(__exec), std::move(__first), __n, __type_identity<_ValueType>());
}

template <class _ExecutionPolicy,
          class _NoThrowForwardIterator,
          class _Size,
          class _RawPolicy                                  = __remove_cvref_t<_ExecutionPolicy>,
          __enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI _NoThrowForwardIterator
uninitialized_default_construct_n(_ExecutionPolicy&& __exec, _NoThrowForwardIterator __first, _Size __n) {
  using _ValueType      = typename iterator_traits<_NoThrowForwardIterator>::value_type;
  using _Implementation = __pstl::__dispatch<__pstl::__memory_default_construct_n, __pstl::__current_configuration, _RawPolicy>;
  return __pstl::__handle_exception<_Implementation>(
      std::forward<_ExecutionPolicy>(__exec), std::move(__first), __n, __type_identity<_ValueType>());
}

// uninitialized_value_construct

template <class _ExecutionPolicy,
          class _NoThrowForwardIterator,
          class _RawPolicy                                  = __remove_cvref_t<_ExecutionPolicy>,
          __enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI void
uninitialized_value_construct(_ExecutionPolicy&& __exec, _NoThrowForwardIterator __first, _NoThrowForwardIterator __last) {
  using _ValueType      = typename iterator_traits<_NoThrowForwardIterator>::value_type;
  using _Implementation = __pstl::__dispatch<__pstl::__memory_value_construct_n, __pstl::__current_configuration, _RawPolicy>;
  auto __n              = std::distance(__first, __last);
  (void)__pstl::__handle_exception<_Implementation>(
      std::forward<_ExecutionPolicy>(__exec), std::move(__first), __n, __type_identity<_ValueType>());
}

template <class _ExecutionPolicy,
          class _NoThrowForwardIterator,
          class _Size,
          class _RawPolicy                                  = __remove_cvref_t<_ExecutionPolicy>,
          __enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI _NoThrowForwardIterator
uninitialized_value_construct_n(_ExecutionPolicy&& __exec, _NoThrowForwardIterator __first, _Size __n) {
  using _ValueType      = typename iterator_traits<_NoThrowForwardIterator>::value_type;
  using _Implementation = __pstl::__dispatch<__pstl::__memory_value_construct_n, __pstl::__current_configuration, _RawPolicy>;
  return __pstl::__handle_exception<_Implementation>(
      std::forward<_ExecutionPolicy>(__exec), std::move(__first), __n, __type_identity<_ValueType>());
}

// uninitialized_copy

template <class _ExecutionPolicy,
          class _ForwardIterator,
          class _NoThrowForwardIterator,
          class _RawPolicy                                  = __remove_cvref_t<_ExecutionPolicy>,
          __enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI _NoThrowForwardIterator uninitialized_copy(
    _ExecutionPolicy&& __exec, _ForwardIterator __first, _ForwardIterator __last, _NoThrowForwardIterator __result) {
  using _ValueType      = typename iterator_traits<_NoThrowForwardIterator>::value_type;
  using _Implementation = __pstl::__dispatch<__pstl::__memory_copy_n, __pstl::__current_configuration, _RawPolicy>;
  auto __n              = std::distance(__first, __last);
  return __pstl::__handle_exception<_Implementation>(
             std::forward<_ExecutionPolicy>(__exec),
             std::move(__first),
             __n,
             std::move(__result),
             __type_identity<_ValueType>())
      .second;
}

template <class _ExecutionPolicy,
          class _ForwardIterator,
          class _Size,
          class _NoThrowForwardIterator,
          class _RawPolicy                                  = __remove_cvref_t<_ExecutionPolicy>,
          __enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI _NoThrowForwardIterator
uninitialized_copy_n(_ExecutionPolicy&& __exec, _ForwardIterator __first, _Size __n, _NoThrowForwardIterator __result) {
  using _ValueType      = typename iterator_traits<_NoThrowForwardIterator>::value_type;
  using _Implementation = __pstl::__dispatch<__pstl::__memory_copy_n, __pstl::__current_configuration, _RawPolicy>;
  return __pstl::__handle_exception<_Implementation>(
             std::forward<_ExecutionPolicy>(__exec),
             std::move(__first),
             __n,
             std::move(__result),
             __type_identity<_ValueType>())
      .second;
}

// uninitialized_move

template <class _ExecutionPolicy,
          class _ForwardIterator,
          class _NoThrowForwardIterator,
          class _RawPolicy                                  = __remove_cvref_t<_ExecutionPolicy>,
          __enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI _NoThrowForwardIterator uninitialized_move(
    _ExecutionPolicy&& __exec, _ForwardIterator __first, _ForwardIterator __last, _NoThrowForwardIterator __result) {
  using _ValueType      = typename iterator_traits<_NoThrowForwardIterator>::value_type;
  using _Implementation = __pstl::__dispatch<__pstl::__memory_move_n, __pstl::__current_configuration, _RawPolicy>;
  auto __n              = std::distance(__first, __last);
  return __pstl::__handle_exception<_Implementation>(
             std::forward<_ExecutionPolicy>(__exec),
             std::move(__first),
             __n,
             std::move(__result),
             __type_identity<_ValueType>(),
             [](auto&& __iter) -> decltype(auto) { return std::move(*__iter); })
      .second;
}

template <class _ExecutionPolicy,
          class _ForwardIterator,
          class _Size,
          class _NoThrowForwardIterator,
          class _RawPolicy                                  = __remove_cvref_t<_ExecutionPolicy>,
          __enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI pair<_ForwardIterator, _NoThrowForwardIterator>
uninitialized_move_n(_ExecutionPolicy&& __exec, _ForwardIterator __first, _Size __n, _NoThrowForwardIterator __result) {
  using _ValueType      = typename iterator_traits<_NoThrowForwardIterator>::value_type;
  using _Implementation = __pstl::__dispatch<__pstl::__memory_move_n, __pstl::__current_configuration, _RawPolicy>;
  return __pstl::__handle_exception<_Implementation>(
      std::forward<_ExecutionPolicy>(__exec),
      std::move(__first),
      __n,
      std::move(__result),
      __type_identity<_ValueType>(),
      [](auto&& __iter) -> decltype(auto) { return std::move(*__iter); });
}

// uninitialized_fill

template <class _ExecutionPolicy,
          class _NoThrowForwardIterator,
          class _Tp
#  if _LIBCPP_STD_VER >= 26
          = typename iterator_traits<_NoThrowForwardIterator>::value_type // P2248R8
#  endif
          ,
          class _RawPolicy                                  = __remove_cvref_t<_ExecutionPolicy>,
          __enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI void uninitialized_fill(
    _ExecutionPolicy&& __exec, _NoThrowForwardIterator __first, _NoThrowForwardIterator __last, const _Tp& __x) {
  using _ValueType      = typename iterator_traits<_NoThrowForwardIterator>::value_type;
  using _Implementation = __pstl::__dispatch<__pstl::__memory_fill_n, __pstl::__current_configuration, _RawPolicy>;
  auto __n              = std::distance(__first, __last);
  (void)__pstl::__handle_exception<_Implementation>(
      std::forward<_ExecutionPolicy>(__exec), std::move(__first), __n, __x, __type_identity<_ValueType>());
}

template <class _ExecutionPolicy,
          class _NoThrowForwardIterator,
          class _Size,
          class _Tp
#  if _LIBCPP_STD_VER >= 26
          = typename iterator_traits<_NoThrowForwardIterator>::value_type // P2248R8
#  endif
          ,
          class _RawPolicy                                  = __remove_cvref_t<_ExecutionPolicy>,
          __enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI _NoThrowForwardIterator
uninitialized_fill_n(_ExecutionPolicy&& __exec, _NoThrowForwardIterator __first, _Size __n, const _Tp& __x) {
  using _ValueType      = typename iterator_traits<_NoThrowForwardIterator>::value_type;
  using _Implementation = __pstl::__dispatch<__pstl::__memory_fill_n, __pstl::__current_configuration, _RawPolicy>;
  return __pstl::__handle_exception<_Implementation>(
      std::forward<_ExecutionPolicy>(__exec), std::move(__first), __n, __x, __type_identity<_ValueType>());
}

// destroy

template <class _ExecutionPolicy,
          class _NoThrowForwardIterator,
          class _RawPolicy                                  = __remove_cvref_t<_ExecutionPolicy>,
          __enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI void
destroy(_ExecutionPolicy&& __exec, _NoThrowForwardIterator __first, _NoThrowForwardIterator __last) {
  using _Implementation = __pstl::__dispatch<__pstl::__memory_destroy_n, __pstl::__current_configuration, _RawPolicy>;
  auto __n              = std::distance(__first, __last);
  (void)__pstl::__handle_exception<_Implementation>(std::forward<_ExecutionPolicy>(__exec), std::move(__first), __n);
}

template <class _ExecutionPolicy,
          class _NoThrowForwardIterator,
          class _Size,
          class _RawPolicy                                  = __remove_cvref_t<_ExecutionPolicy>,
          __enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI _NoThrowForwardIterator
destroy_n(_ExecutionPolicy&& __exec, _NoThrowForwardIterator __first, _Size __n) {
  using _Implementation = __pstl::__dispatch<__pstl::__memory_destroy_n, __pstl::__current_configuration, _RawPolicy>;
  return __pstl::__handle_exception<_Implementation>(std::forward<_ExecutionPolicy>(__exec), std::move(__first), __n);
}

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_STD_VER >= 17 && _LIBCPP_HAS_EXPERIMENTAL_PSTL && !defined(_LIBCPP_FREESTANDING)

_LIBCPP_POP_MACROS

#endif // _LIBCPP___MEMORY_PSTL_UNINITIALIZED_ALGORITHMS_H
