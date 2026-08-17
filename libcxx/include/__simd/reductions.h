// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___SIMD_REDUCTIONS_H
#define _LIBCPP___SIMD_REDUCTIONS_H

#include <__concepts/same_as.h>
#include <__concepts/totally_ordered.h>
#include <__config>
#include <__cstddef/size_t.h>
#include <__functional/operations.h>
#include <__simd/abi.h>
#include <__simd/basic_mask.h>
#include <__simd/basic_vec.h>
#include <__simd/concepts.h>
#include <__simd/vectorizable.h>
#include <__type_traits/type_identity.h>
#include <limits>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

namespace simd {

// ======================= [simd.mask.reductions] =======================
//
// Defined ahead of the vec reductions below because reduce_min/reduce_max's masked overloads use
// none_of.

template <size_t _Bytes, class _Abi>
_LIBCPP_HIDE_FROM_ABI constexpr bool all_of(const basic_mask<_Bytes, _Abi>& __k) noexcept {
  for (__simd_size_type __i = 0; __i != basic_mask<_Bytes, _Abi>::size(); ++__i)
    if (!__k[__i])
      return false;
  return true;
}

template <size_t _Bytes, class _Abi>
_LIBCPP_HIDE_FROM_ABI constexpr bool any_of(const basic_mask<_Bytes, _Abi>& __k) noexcept {
  for (__simd_size_type __i = 0; __i != basic_mask<_Bytes, _Abi>::size(); ++__i)
    if (__k[__i])
      return true;
  return false;
}

template <size_t _Bytes, class _Abi>
_LIBCPP_HIDE_FROM_ABI constexpr bool none_of(const basic_mask<_Bytes, _Abi>& __k) noexcept {
  return !any_of(__k);
}

template <size_t _Bytes, class _Abi>
_LIBCPP_HIDE_FROM_ABI constexpr __simd_size_type reduce_count(const basic_mask<_Bytes, _Abi>& __k) noexcept {
  __simd_size_type __result = 0;
  for (__simd_size_type __i = 0; __i != basic_mask<_Bytes, _Abi>::size(); ++__i)
    __result += __k[__i];
  return __result;
}

// [simd.mask.reductions]/5-6: Preconditions any_of(k).
template <size_t _Bytes, class _Abi>
_LIBCPP_HIDE_FROM_ABI constexpr __simd_size_type reduce_min_index(const basic_mask<_Bytes, _Abi>& __k) {
  _LIBCPP_ASSERT_UNCATEGORIZED(any_of(__k), "simd::reduce_min_index: no element of the mask is set");
  for (__simd_size_type __i = 0; __i != basic_mask<_Bytes, _Abi>::size(); ++__i)
    if (__k[__i])
      return __i;
  __builtin_unreachable();
}

template <size_t _Bytes, class _Abi>
_LIBCPP_HIDE_FROM_ABI constexpr __simd_size_type reduce_max_index(const basic_mask<_Bytes, _Abi>& __k) {
  _LIBCPP_ASSERT_UNCATEGORIZED(any_of(__k), "simd::reduce_max_index: no element of the mask is set");
  for (__simd_size_type __i = basic_mask<_Bytes, _Abi>::size(); __i != 0; --__i)
    if (__k[__i - 1])
      return __i - 1;
  __builtin_unreachable();
}

// [simd.mask.reductions]/9-12: scalar-bool overloads.
_LIBCPP_HIDE_FROM_ABI constexpr bool all_of(same_as<bool> auto __x) noexcept { return __x; }
_LIBCPP_HIDE_FROM_ABI constexpr bool any_of(same_as<bool> auto __x) noexcept { return __x; }
_LIBCPP_HIDE_FROM_ABI constexpr bool none_of(same_as<bool> auto __x) noexcept { return !__x; }
_LIBCPP_HIDE_FROM_ABI constexpr __simd_size_type reduce_count(same_as<bool> auto __x) noexcept { return __x; }

_LIBCPP_HIDE_FROM_ABI constexpr __simd_size_type reduce_min_index(same_as<bool> auto __x) {
  _LIBCPP_ASSERT_UNCATEGORIZED(__x, "simd::reduce_min_index: mask is false");
  (void)__x;
  return 0;
}

_LIBCPP_HIDE_FROM_ABI constexpr __simd_size_type reduce_max_index(same_as<bool> auto __x) {
  _LIBCPP_ASSERT_UNCATEGORIZED(__x, "simd::reduce_max_index: mask is false");
  (void)__x;
  return 0;
}

// ======================= [simd.reductions] =======================
//
// [simd.reductions]/9 and /14: the identity element defaults for the five special-cased binary
// operations. Encoded via two overloads distinguished by __is_known_reduction_op: one keeps the
// defaulted `identity_element` parameter (only reachable for those five operations), the other has
// no default and therefore requires the caller to supply one, matching the clause's Constraints:
// "An argument for identity_element is provided ... unless BinaryOperation is one of plus<>,
// multiplies<>, bit_and<>, bit_or<>, or bit_xor<>."
template <class _Op>
inline constexpr bool __is_known_reduction_op = false;
template <>
inline constexpr bool __is_known_reduction_op<plus<>> = true;
template <>
inline constexpr bool __is_known_reduction_op<multiplies<>> = true;
template <>
inline constexpr bool __is_known_reduction_op<bit_and<>> = true;
template <>
inline constexpr bool __is_known_reduction_op<bit_or<>> = true;
template <>
inline constexpr bool __is_known_reduction_op<bit_xor<>> = true;

template <class _Op, class _Tp>
_LIBCPP_HIDE_FROM_ABI consteval _Tp __reduction_identity() {
  if constexpr (same_as<_Op, multiplies<>>) {
    return _Tp(1);
  } else if constexpr (same_as<_Op, bit_and<>>) {
    return _Tp(~_Tp());
  } else {
    return _Tp(); // plus<>, bit_or<>, bit_xor<>
  }
}

template <class _Tp, class _Abi, class _BinaryOperation = plus<>>
  requires __reduction_binary_operation<_BinaryOperation, _Tp>
_LIBCPP_HIDE_FROM_ABI constexpr _Tp reduce(const basic_vec<_Tp, _Abi>& __x, _BinaryOperation __binary_op = {}) {
  _Tp __result = __x[0];
  for (__simd_size_type __i = 1; __i != basic_vec<_Tp, _Abi>::size(); ++__i)
    __result = __binary_op(__result, __x[__i]);
  return __result;
}

template <class _Tp, class _Abi, class _BinaryOperation = plus<>>
  requires(__reduction_binary_operation<_BinaryOperation, _Tp> && __is_known_reduction_op<_BinaryOperation>)
_LIBCPP_HIDE_FROM_ABI constexpr _Tp
reduce(const basic_vec<_Tp, _Abi>& __x, const typename basic_vec<_Tp, _Abi>::mask_type& __mask,
       _BinaryOperation __binary_op = {}, type_identity_t<_Tp> __identity_element = __reduction_identity<_BinaryOperation, _Tp>()) {
  if (none_of(__mask))
    return __identity_element;
  _Tp __result = __identity_element;
  bool __seen  = false;
  for (__simd_size_type __i = 0; __i != basic_vec<_Tp, _Abi>::size(); ++__i)
    if (__mask[__i])
      __result = __seen ? __binary_op(__result, __x[__i]) : (__seen = true, __x[__i]);
  return __result;
}

template <class _Tp, class _Abi, class _BinaryOperation>
  requires(__reduction_binary_operation<_BinaryOperation, _Tp> && !__is_known_reduction_op<_BinaryOperation>)
_LIBCPP_HIDE_FROM_ABI constexpr _Tp reduce(const basic_vec<_Tp, _Abi>& __x,
                                           const typename basic_vec<_Tp, _Abi>::mask_type& __mask,
                                           _BinaryOperation __binary_op, type_identity_t<_Tp> __identity_element) {
  if (none_of(__mask))
    return __identity_element;
  _Tp __result = __identity_element;
  for (__simd_size_type __i = 0; __i != basic_vec<_Tp, _Abi>::size(); ++__i)
    if (__mask[__i])
      __result = __binary_op(__result, __x[__i]);
  return __result;
}

// [simd.reductions]/10-14: scalar overloads.
template <class _Tp, class _BinaryOperation = plus<>>
  requires(__vectorizable<_Tp> && __reduction_binary_operation<_BinaryOperation, _Tp>)
_LIBCPP_HIDE_FROM_ABI constexpr _Tp reduce(const _Tp& __x, _BinaryOperation = {}) {
  return __x;
}

template <class _Tp, class _BinaryOperation = plus<>>
  requires(__vectorizable<_Tp> && __reduction_binary_operation<_BinaryOperation, _Tp> &&
           __is_known_reduction_op<_BinaryOperation>)
_LIBCPP_HIDE_FROM_ABI constexpr _Tp reduce(const _Tp& __x, same_as<bool> auto __mask, _BinaryOperation = {},
                                           type_identity_t<_Tp> __identity_element =
                                               __reduction_identity<_BinaryOperation, _Tp>()) {
  return __mask ? __x : __identity_element;
}

template <class _Tp, class _BinaryOperation>
  requires(__vectorizable<_Tp> && __reduction_binary_operation<_BinaryOperation, _Tp> &&
           !__is_known_reduction_op<_BinaryOperation>)
_LIBCPP_HIDE_FROM_ABI constexpr _Tp
reduce(const _Tp& __x, same_as<bool> auto __mask, _BinaryOperation, type_identity_t<_Tp> __identity_element) {
  return __mask ? __x : __identity_element;
}

// [simd.reductions]/15-22: reduce_min/reduce_max.
template <class _Tp, class _Abi>
  requires totally_ordered<_Tp>
_LIBCPP_HIDE_FROM_ABI constexpr _Tp reduce_min(const basic_vec<_Tp, _Abi>& __x) noexcept {
  _Tp __result = __x[0];
  for (__simd_size_type __i = 1; __i != basic_vec<_Tp, _Abi>::size(); ++__i)
    if (__x[__i] < __result)
      __result = __x[__i];
  return __result;
}

template <class _Tp, class _Abi>
  requires totally_ordered<_Tp>
_LIBCPP_HIDE_FROM_ABI constexpr _Tp reduce_min(const basic_vec<_Tp, _Abi>& __x,
                                               const typename basic_vec<_Tp, _Abi>::mask_type& __mask) noexcept {
  if (none_of(__mask))
    return numeric_limits<_Tp>::max();
  _Tp __result{};
  bool __seen = false;
  for (__simd_size_type __i = 0; __i != basic_vec<_Tp, _Abi>::size(); ++__i)
    if (__mask[__i] && (!__seen || __x[__i] < __result))
      __result = __x[__i], __seen = true;
  return __result;
}

template <class _Tp, class _Abi>
  requires totally_ordered<_Tp>
_LIBCPP_HIDE_FROM_ABI constexpr _Tp reduce_max(const basic_vec<_Tp, _Abi>& __x) noexcept {
  _Tp __result = __x[0];
  for (__simd_size_type __i = 1; __i != basic_vec<_Tp, _Abi>::size(); ++__i)
    if (__result < __x[__i])
      __result = __x[__i];
  return __result;
}

template <class _Tp, class _Abi>
  requires totally_ordered<_Tp>
_LIBCPP_HIDE_FROM_ABI constexpr _Tp reduce_max(const basic_vec<_Tp, _Abi>& __x,
                                               const typename basic_vec<_Tp, _Abi>::mask_type& __mask) noexcept {
  if (none_of(__mask))
    return numeric_limits<_Tp>::lowest();
  _Tp __result{};
  bool __seen = false;
  for (__simd_size_type __i = 0; __i != basic_vec<_Tp, _Abi>::size(); ++__i)
    if (__mask[__i] && (!__seen || __result < __x[__i]))
      __result = __x[__i], __seen = true;
  return __result;
}

// [simd.reductions]/23-25: scalar overloads.
template <class _Tp>
  requires(__vectorizable<_Tp> && totally_ordered<_Tp>)
_LIBCPP_HIDE_FROM_ABI constexpr _Tp reduce_min(const _Tp& __x) noexcept {
  return __x;
}

template <class _Tp>
  requires(__vectorizable<_Tp> && totally_ordered<_Tp>)
_LIBCPP_HIDE_FROM_ABI constexpr _Tp reduce_min(const _Tp& __x, same_as<bool> auto __mask) noexcept {
  return __mask ? __x : numeric_limits<_Tp>::max();
}

template <class _Tp>
  requires(__vectorizable<_Tp> && totally_ordered<_Tp>)
_LIBCPP_HIDE_FROM_ABI constexpr _Tp reduce_max(const _Tp& __x) noexcept {
  return __x;
}

template <class _Tp>
  requires(__vectorizable<_Tp> && totally_ordered<_Tp>)
_LIBCPP_HIDE_FROM_ABI constexpr _Tp reduce_max(const _Tp& __x, same_as<bool> auto __mask) noexcept {
  return __mask ? __x : numeric_limits<_Tp>::lowest();
}

} // namespace simd

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP___SIMD_REDUCTIONS_H
