//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#pragma GCC system_header

#ifndef _LIBCPP___ALGORITHM_PSTL_FREESTANDING_H
#define _LIBCPP___ALGORITHM_PSTL_FREESTANDING_H

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
          class _Predicate,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI bool
any_of(_ExecutionPolicy&& __policy, _ForwardIterator __first, _ForwardIterator __last, _Predicate __pred) = delete;

template <class _ExecutionPolicy,
          class _ForwardIterator,
          class _Pred,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI bool
all_of(_ExecutionPolicy&& __policy, _ForwardIterator __first, _ForwardIterator __last, _Pred __pred) = delete;

template <class _ExecutionPolicy,
          class _ForwardIterator,
          class _Pred,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI bool
none_of(_ExecutionPolicy&& __policy, _ForwardIterator __first, _ForwardIterator __last, _Pred __pred) = delete;

template <class _ExecutionPolicy,
          class _ForwardIterator,
          class _ForwardOutIterator,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI _ForwardOutIterator
copy(_ExecutionPolicy&& __policy, _ForwardIterator __first, _ForwardIterator __last, _ForwardOutIterator __result) = delete;

template <class _ExecutionPolicy,
          class _ForwardIterator,
          class _ForwardOutIterator,
          class _Size,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI _ForwardOutIterator
copy_n(_ExecutionPolicy&& __policy, _ForwardIterator __first, _Size __n, _ForwardOutIterator __result) = delete;

template <class _ExecutionPolicy,
          class _ForwardIterator,
          class _Predicate,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI __iterator_difference_type<_ForwardIterator>
count_if(_ExecutionPolicy&& __policy, _ForwardIterator __first, _ForwardIterator __last, _Predicate __pred) = delete;

template <class _ExecutionPolicy,
          class _ForwardIterator,
          class _Tp
#  if _LIBCPP_STD_VER >= 26
          = typename iterator_traits<_ForwardIterator>::value_type
#  endif
          ,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI __iterator_difference_type<_ForwardIterator>
count(_ExecutionPolicy&& __policy, _ForwardIterator __first, _ForwardIterator __last, const _Tp& __value) = delete;

template <class _ExecutionPolicy,
          class _ForwardIterator1,
          class _ForwardIterator2,
          class _Pred,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI bool
equal(_ExecutionPolicy&& __policy,
      _ForwardIterator1 __first1,
      _ForwardIterator1 __last1,
      _ForwardIterator2 __first2,
      _Pred __pred) = delete;

template <class _ExecutionPolicy, class _ForwardIterator, class _Comp,
          class _RawPolicy = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI _ForwardIterator
min_element(_ExecutionPolicy&& __policy, _ForwardIterator __first, _ForwardIterator __last, _Comp __comp) = delete;

template <class _ExecutionPolicy, class _ForwardIterator,
          class _RawPolicy = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI _ForwardIterator
min_element(_ExecutionPolicy&& __policy, _ForwardIterator __first, _ForwardIterator __last) = delete;

template <class _ExecutionPolicy, class _ForwardIterator, class _Comp,
          class _RawPolicy = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI _ForwardIterator
max_element(_ExecutionPolicy&& __policy, _ForwardIterator __first, _ForwardIterator __last, _Comp __comp) = delete;

template <class _ExecutionPolicy, class _ForwardIterator,
          class _RawPolicy = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI _ForwardIterator
max_element(_ExecutionPolicy&& __policy, _ForwardIterator __first, _ForwardIterator __last) = delete;

template <class _ExecutionPolicy, class _ForwardIterator, class _Comp,
          class _RawPolicy = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI pair<_ForwardIterator, _ForwardIterator>
minmax_element(_ExecutionPolicy&& __policy, _ForwardIterator __first, _ForwardIterator __last, _Comp __comp) = delete;

template <class _ExecutionPolicy, class _ForwardIterator,
          class _RawPolicy = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI pair<_ForwardIterator, _ForwardIterator>
minmax_element(_ExecutionPolicy&& __policy, _ForwardIterator __first, _ForwardIterator __last) = delete;

template <class _ExecutionPolicy, class _ForwardIterator1, class _ForwardIterator2, class _Comp,
          class _RawPolicy = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI bool
lexicographical_compare(_ExecutionPolicy&& __policy,
                        _ForwardIterator1 __first1,
                        _ForwardIterator1 __last1,
                        _ForwardIterator2 __first2,
                        _ForwardIterator2 __last2,
                        _Comp __comp) = delete;

template <class _ExecutionPolicy, class _ForwardIterator1, class _ForwardIterator2,
          class _RawPolicy = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI bool
lexicographical_compare(_ExecutionPolicy&& __policy,
                        _ForwardIterator1 __first1,
                        _ForwardIterator1 __last1,
                        _ForwardIterator2 __first2,
                        _ForwardIterator2 __last2) = delete;

template <class _ExecutionPolicy,
          class _ForwardIterator1,
          class _ForwardIterator2,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI bool
equal(_ExecutionPolicy&& __policy, _ForwardIterator1 __first1, _ForwardIterator1 __last1, _ForwardIterator2 __first2) = delete;

template <class _ExecutionPolicy,
          class _ForwardIterator1,
          class _ForwardIterator2,
          class _Pred,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI bool
equal(_ExecutionPolicy&& __policy,
      _ForwardIterator1 __first1,
      _ForwardIterator1 __last1,
      _ForwardIterator2 __first2,
      _ForwardIterator2 __last2,
      _Pred __pred) = delete;

template <class _ExecutionPolicy,
          class _ForwardIterator1,
          class _ForwardIterator2,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI bool
equal(_ExecutionPolicy&& __policy,
      _ForwardIterator1 __first1,
      _ForwardIterator1 __last1,
      _ForwardIterator2 __first2,
      _ForwardIterator2 __last2) = delete;

template <class _ExecutionPolicy,
          class _ForwardIterator,
          class _Tp
#  if _LIBCPP_STD_VER >= 26
          = typename iterator_traits<_ForwardIterator>::value_type
#  endif
          ,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI void
fill(_ExecutionPolicy&& __policy, _ForwardIterator __first, _ForwardIterator __last, const _Tp& __value) = delete;

template <class _ExecutionPolicy,
          class _ForwardIterator,
          class _Size,
          class _Tp
#  if _LIBCPP_STD_VER >= 26
          = typename iterator_traits<_ForwardIterator>::value_type
#  endif
          ,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI void
fill_n(_ExecutionPolicy&& __policy, _ForwardIterator __first, _Size __n, const _Tp& __value) = delete;

template <class _ExecutionPolicy,
          class _ForwardIterator,
          class _Predicate,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI _ForwardIterator
find_if(_ExecutionPolicy&& __policy, _ForwardIterator __first, _ForwardIterator __last, _Predicate __pred) = delete;

template <class _ExecutionPolicy,
          class _ForwardIterator,
          class _Predicate,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI _ForwardIterator
find_if_not(_ExecutionPolicy&& __policy, _ForwardIterator __first, _ForwardIterator __last, _Predicate __pred) = delete;

template <class _ExecutionPolicy,
          class _ForwardIterator,
          class _Tp
#  if _LIBCPP_STD_VER >= 26
          = typename iterator_traits<_ForwardIterator>::value_type
#  endif
          ,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI _ForwardIterator
find(_ExecutionPolicy&& __policy, _ForwardIterator __first, _ForwardIterator __last, const _Tp& __value) = delete;

template <class _ExecutionPolicy, class _RandomAccessIterator, class _RandomAccessIterator2, class _BinaryPredicate,
          class _RawPolicy = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI _RandomAccessIterator
find_end(_ExecutionPolicy&& __policy, _RandomAccessIterator __first, _RandomAccessIterator __last,
         _RandomAccessIterator2 __first2, _RandomAccessIterator2 __last2, _BinaryPredicate __pred) = delete;

template <class _ExecutionPolicy, class _RandomAccessIterator, class _RandomAccessIterator2, class _BinaryPredicate,
          class _RawPolicy = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI _RandomAccessIterator
find_first_of(_ExecutionPolicy&& __policy, _RandomAccessIterator __first, _RandomAccessIterator __last,
              _RandomAccessIterator2 __first2, _RandomAccessIterator2 __last2, _BinaryPredicate __pred) = delete;

template <class _ExecutionPolicy, class _RandomAccessIterator, class _BinaryPredicate,
          class _RawPolicy = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI _RandomAccessIterator
adjacent_find(_ExecutionPolicy&& __policy, _RandomAccessIterator __first, _RandomAccessIterator __last,
              _BinaryPredicate __pred) = delete;

template <class _ExecutionPolicy, class _RandomAccessIterator1, class _RandomAccessIterator2, class _BinaryPredicate,
          class _RawPolicy = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI pair<_RandomAccessIterator1, _RandomAccessIterator2>
mismatch(_ExecutionPolicy&& __policy, _RandomAccessIterator1 __first1, _RandomAccessIterator1 __last1,
         _RandomAccessIterator2 __first2, _RandomAccessIterator2 __last2, _BinaryPredicate __pred) = delete;

template <class _ExecutionPolicy, class _RandomAccessIterator1, class _RandomAccessIterator2, class _BinaryPredicate,
          class _RawPolicy = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI _RandomAccessIterator1
search(_ExecutionPolicy&& __policy, _RandomAccessIterator1 __first1, _RandomAccessIterator1 __last1,
       _RandomAccessIterator2 __first2, _RandomAccessIterator2 __last2, _BinaryPredicate __pred) = delete;

template <class _ExecutionPolicy,
          class _ForwardIterator,
          class _Function,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI void
for_each(_ExecutionPolicy&& __policy, _ForwardIterator __first, _ForwardIterator __last, _Function __func) = delete;

template <class _ExecutionPolicy,
          class _ForwardIterator,
          class _Size,
          class _Function,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI void
for_each_n(_ExecutionPolicy&& __policy, _ForwardIterator __first, _Size __size, _Function __func) = delete;

template <class _ExecutionPolicy,
          class _ForwardIterator,
          class _Generator,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI void
generate(_ExecutionPolicy&& __policy, _ForwardIterator __first, _ForwardIterator __last, _Generator __gen) = delete;

template <class _ExecutionPolicy,
          class _ForwardIterator,
          class _Size,
          class _Generator,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI void
generate_n(_ExecutionPolicy&& __policy, _ForwardIterator __first, _Size __n, _Generator __gen) = delete;

template <class _ExecutionPolicy,
          class _ForwardIterator,
          class _Predicate,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI bool
is_partitioned(_ExecutionPolicy&& __policy, _ForwardIterator __first, _ForwardIterator __last, _Predicate __pred) = delete;

template <class _ExecutionPolicy, class _ForwardIterator, class _Tp,
          class _RawPolicy = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI _ForwardIterator
remove(_ExecutionPolicy&& __policy, _ForwardIterator __first, _ForwardIterator __last, const _Tp& __value) = delete;

template <class _ExecutionPolicy, class _ForwardIterator, class _Predicate,
          class _RawPolicy = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI _ForwardIterator
remove_if(_ExecutionPolicy&& __policy, _ForwardIterator __first, _ForwardIterator __last, _Predicate __pred) = delete;

template <class _ExecutionPolicy, class _ForwardIterator, class _BinaryPredicate,
          class _RawPolicy = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI _ForwardIterator
unique(_ExecutionPolicy&& __policy, _ForwardIterator __first, _ForwardIterator __last, _BinaryPredicate __pred) = delete;

template <class _ExecutionPolicy, class _ForwardIterator, class _RawPolicy = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI _ForwardIterator
unique(_ExecutionPolicy&& __policy, _ForwardIterator __first, _ForwardIterator __last) = delete;

template <class _ExecutionPolicy, class _BidirectionalIterator,
          class _RawPolicy = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI void
reverse(_ExecutionPolicy&& __policy, _BidirectionalIterator __first, _BidirectionalIterator __last) = delete;

template <class _ExecutionPolicy, class _ForwardIterator, class _RawPolicy = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI _ForwardIterator rotate(
    _ExecutionPolicy&& __policy, _ForwardIterator __first, _ForwardIterator __middle, _ForwardIterator __last) = delete;

#if _LIBCPP_STD_VER >= 20
template <class _ExecutionPolicy, class _ForwardIterator, class _Distance,
          class _RawPolicy = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI _ForwardIterator shift_left(
    _ExecutionPolicy&& __policy, _ForwardIterator __first, _ForwardIterator __last, _Distance __n) = delete;

template <class _ExecutionPolicy, class _ForwardIterator, class _Distance,
          class _RawPolicy = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI _ForwardIterator shift_right(
    _ExecutionPolicy&& __policy, _ForwardIterator __first, _ForwardIterator __last, _Distance __n) = delete;
#endif

template <class _ExecutionPolicy, class _ForwardIterator1, class _ForwardIterator2,
          class _RawPolicy = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI _ForwardIterator2 swap_ranges(_ExecutionPolicy&& __policy,
                                                    _ForwardIterator1 __first1,
                                                    _ForwardIterator1 __last1,
                                                    _ForwardIterator2 __first2) = delete;

template <class _ExecutionPolicy, class _ForwardIterator, class _Predicate,
          class _RawPolicy = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI _ForwardIterator partition(
    _ExecutionPolicy&& __policy, _ForwardIterator __first, _ForwardIterator __last, _Predicate __pred) = delete;

template <class _ExecutionPolicy,
          class _ForwardIterator1,
          class _ForwardIterator2,
          class _ForwardOutIterator,
          class _Comp,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI _ForwardOutIterator
merge(_ExecutionPolicy&& __policy,
      _ForwardIterator1 __first1,
      _ForwardIterator1 __last1,
      _ForwardIterator2 __first2,
      _ForwardIterator2 __last2,
      _ForwardOutIterator __result,
      _Comp __comp) = delete;

template <class _ExecutionPolicy,
          class _ForwardIterator1,
          class _ForwardIterator2,
          class _ForwardOutIterator,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI _ForwardOutIterator
merge(_ExecutionPolicy&& __policy,
      _ForwardIterator1 __first1,
      _ForwardIterator1 __last1,
      _ForwardIterator2 __first2,
      _ForwardIterator2 __last2,
      _ForwardOutIterator __result) = delete;

template <class _ExecutionPolicy, class _RandomAccessIterator, class _Comp,
          class _RawPolicy = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI bool is_heap(
    _ExecutionPolicy&& __policy, _RandomAccessIterator __first, _RandomAccessIterator __last, _Comp __comp) = delete;

template <class _ExecutionPolicy, class _RandomAccessIterator,
          class _RawPolicy = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI bool is_heap(
    _ExecutionPolicy&& __policy, _RandomAccessIterator __first, _RandomAccessIterator __last) = delete;

template <class _ExecutionPolicy, class _RandomAccessIterator, class _Comp,
          class _RawPolicy = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI _RandomAccessIterator is_heap_until(
    _ExecutionPolicy&& __policy, _RandomAccessIterator __first, _RandomAccessIterator __last, _Comp __comp) = delete;

template <class _ExecutionPolicy, class _RandomAccessIterator,
          class _RawPolicy = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI _RandomAccessIterator is_heap_until(
    _ExecutionPolicy&& __policy, _RandomAccessIterator __first, _RandomAccessIterator __last) = delete;

template <class _ExecutionPolicy,
          class _ForwardIterator,
          class _ForwardOutIterator,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI _ForwardOutIterator
move(_ExecutionPolicy&& __policy, _ForwardIterator __first, _ForwardIterator __last, _ForwardOutIterator __result) = delete;

template <class _ExecutionPolicy,
          class _ForwardIterator,
          class _Pred,
          class _Tp
#  if _LIBCPP_STD_VER >= 26
          = typename iterator_traits<_ForwardIterator>::value_type
#  endif
          ,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI void
replace_if(_ExecutionPolicy&& __policy,
           _ForwardIterator __first,
           _ForwardIterator __last,
           _Pred __pred,
           const _Tp& __new_value) = delete;

template <class _ExecutionPolicy,
          class _ForwardIterator,
          class _Tp
#  if _LIBCPP_STD_VER >= 26
          = typename iterator_traits<_ForwardIterator>::value_type
#  endif
          ,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI void
replace(_ExecutionPolicy&& __policy,
        _ForwardIterator __first,
        _ForwardIterator __last,
        const _Tp& __old_value,
        const _Tp& __new_value) = delete;

template <class _ExecutionPolicy,
          class _ForwardIterator,
          class _ForwardOutIterator,
          class _Pred,
          class _Tp
#  if _LIBCPP_STD_VER >= 26
          = typename iterator_traits<_ForwardOutIterator>::value_type
#  endif
          ,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI void replace_copy_if(
    _ExecutionPolicy&& __policy,
    _ForwardIterator __first,
    _ForwardIterator __last,
    _ForwardOutIterator __result,
    _Pred __pred,
    const _Tp& __new_value) = delete;

template <class _ExecutionPolicy,
          class _ForwardIterator,
          class _ForwardOutIterator,
          class _Tp,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI void replace_copy(
    _ExecutionPolicy&& __policy,
    _ForwardIterator __first,
    _ForwardIterator __last,
    _ForwardOutIterator __result,
    const _Tp& __old_value,
    const _Tp& __new_value) = delete;

template <class _ExecutionPolicy,
          class _ForwardIterator,
          class _ForwardOutIterator,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI _ForwardOutIterator rotate_copy(
    _ExecutionPolicy&& __policy,
    _ForwardIterator __first,
    _ForwardIterator __middle,
    _ForwardIterator __last,
    _ForwardOutIterator __result) = delete;

template <class _ExecutionPolicy,
          class _BidirectionalIterator,
          class _ForwardOutIterator,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI _ForwardOutIterator reverse_copy(
    _ExecutionPolicy&& __policy,
    _BidirectionalIterator __first,
    _BidirectionalIterator __last,
    _ForwardOutIterator __result) = delete;

template <class _ExecutionPolicy,
          class _RandomAccessIterator,
          class _Comp,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI void
sort(_ExecutionPolicy&& __policy, _RandomAccessIterator __first, _RandomAccessIterator __last, _Comp __comp) = delete;

template <class _ExecutionPolicy,
          class _RandomAccessIterator,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI void
sort(_ExecutionPolicy&& __policy, _RandomAccessIterator __first, _RandomAccessIterator __last) = delete;

template <class _ExecutionPolicy,
          class _RandomAccessIterator,
          class _Comp,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI void
stable_sort(_ExecutionPolicy&& __policy, _RandomAccessIterator __first, _RandomAccessIterator __last, _Comp __comp) = delete;

template <class _ExecutionPolicy,
          class _RandomAccessIterator,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI void
stable_sort(_ExecutionPolicy&& __policy, _RandomAccessIterator __first, _RandomAccessIterator __last) = delete;

template <class _ExecutionPolicy,
          class _ForwardIterator,
          class _ForwardOutIterator,
          class _UnaryOperation,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI _ForwardOutIterator transform(
    _ExecutionPolicy&& __policy,
    _ForwardIterator __first,
    _ForwardIterator __last,
    _ForwardOutIterator __result,
    _UnaryOperation __op) = delete;

template <class _ExecutionPolicy,
          class _ForwardIterator1,
          class _ForwardIterator2,
          class _ForwardOutIterator,
          class _BinaryOperation,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI _ForwardOutIterator transform(
    _ExecutionPolicy&& __policy,
    _ForwardIterator1 __first1,
    _ForwardIterator1 __last1,
    _ForwardIterator2 __first2,
    _ForwardOutIterator __result,
    _BinaryOperation __op) = delete;

template <class _ExecutionPolicy,
          class _ForwardIterator,
          class _ForwardOutIterator,
          class _Predicate,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI _ForwardOutIterator
copy_if(_ExecutionPolicy&& __policy, _ForwardIterator __first, _ForwardIterator __last, _ForwardOutIterator __result,
        _Predicate __pred) = delete;

template <class _ExecutionPolicy,
          class _ForwardIterator,
          class _Predicate,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI _ForwardIterator
stable_partition(_ExecutionPolicy&& __policy, _ForwardIterator __first, _ForwardIterator __last, _Predicate __pred) = delete;

template <class _ExecutionPolicy,
          class _ForwardIterator,
          class _ForwardOutIterator1,
          class _ForwardOutIterator2,
          class _Predicate,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI pair<_ForwardOutIterator1, _ForwardOutIterator2>
partition_copy(_ExecutionPolicy&& __policy, _ForwardIterator __first, _ForwardIterator __last,
               _ForwardOutIterator1 __out_true, _ForwardOutIterator2 __out_false, _Predicate __pred) = delete;

template <class _ExecutionPolicy,
          class _ForwardIterator,
          class _Size,
          class _Tp,
          class _Predicate,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI _ForwardIterator
search_n(_ExecutionPolicy&& __policy, _ForwardIterator __first, _ForwardIterator __last, _Size __count,
         const _Tp& __value, _Predicate __pred) = delete;

template <class _ExecutionPolicy, class _RandomAccessIterator, class _Compare,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI void
partial_sort(_ExecutionPolicy&& __policy, _RandomAccessIterator __first, _RandomAccessIterator __middle,
             _RandomAccessIterator __last, _Compare __comp) = delete;

template <class _ExecutionPolicy, class _RandomAccessIterator,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI void
partial_sort(_ExecutionPolicy&& __policy, _RandomAccessIterator __first, _RandomAccessIterator __middle,
             _RandomAccessIterator __last) = delete;

template <class _ExecutionPolicy, class _ForwardIterator, class _RandomAccessIterator, class _Compare,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI _RandomAccessIterator
partial_sort_copy(_ExecutionPolicy&& __policy, _ForwardIterator __first, _ForwardIterator __last,
                  _RandomAccessIterator __result_first, _RandomAccessIterator __result_last, _Compare __comp) = delete;

template <class _ExecutionPolicy, class _ForwardIterator, class _RandomAccessIterator,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI _RandomAccessIterator
partial_sort_copy(_ExecutionPolicy&& __policy, _ForwardIterator __first, _ForwardIterator __last,
                  _RandomAccessIterator __result_first, _RandomAccessIterator __result_last) = delete;

template <class _ExecutionPolicy, class _ForwardIterator, class _Compare,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI bool
is_sorted(_ExecutionPolicy&& __policy, _ForwardIterator __first, _ForwardIterator __last, _Compare __comp) = delete;

template <class _ExecutionPolicy, class _ForwardIterator,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI bool
is_sorted(_ExecutionPolicy&& __policy, _ForwardIterator __first, _ForwardIterator __last) = delete;

template <class _ExecutionPolicy, class _RandomAccessIterator, class _Compare,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI void
nth_element(_ExecutionPolicy&& __policy, _RandomAccessIterator __first, _RandomAccessIterator __nth,
            _RandomAccessIterator __last, _Compare __comp) = delete;

template <class _ExecutionPolicy, class _RandomAccessIterator,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI void
nth_element(_ExecutionPolicy&& __policy, _RandomAccessIterator __first, _RandomAccessIterator __nth,
            _RandomAccessIterator __last) = delete;

template <class _ExecutionPolicy, class _BidirectionalIterator, class _Compare,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI void
inplace_merge(_ExecutionPolicy&& __policy, _BidirectionalIterator __first, _BidirectionalIterator __middle,
              _BidirectionalIterator __last, _Compare __comp) = delete;

template <class _ExecutionPolicy, class _BidirectionalIterator,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
_LIBCPP_HIDE_FROM_ABI void
inplace_merge(_ExecutionPolicy&& __policy, _BidirectionalIterator __first, _BidirectionalIterator __middle,
              _BidirectionalIterator __last) = delete;

template <class _ExecutionPolicy, class _ForwardIterator1, class _ForwardIterator2, class _Compare,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI bool
includes(_ExecutionPolicy&& __policy, _ForwardIterator1 __first1, _ForwardIterator1 __last1,
         _ForwardIterator2 __first2, _ForwardIterator2 __last2, _Compare __comp) = delete;

template <class _ExecutionPolicy, class _ForwardIterator1, class _ForwardIterator2,
          class _RawPolicy                                    = __remove_cvref_t<_ExecutionPolicy>,
          enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI bool
includes(_ExecutionPolicy&& __policy, _ForwardIterator1 __first1, _ForwardIterator1 __last1,
         _ForwardIterator2 __first2, _ForwardIterator2 __last2) = delete;
_LIBCPP_END_NAMESPACE_STD
#endif

#endif // _LIBCPP___ALGORITHM_PSTL_FREESTANDING_H
