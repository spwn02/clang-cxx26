// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___SIMD_ITERATOR_H
#define _LIBCPP___SIMD_ITERATOR_H

#include <__compare/ordering.h>
#include <__config>
#include <__iterator/default_sentinel.h>
#include <__iterator/iterator_traits.h>
#include <__memory/addressof.h>
#include <__simd/abi.h>
#include <__type_traits/is_const.h>
#include <__type_traits/remove_const.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

namespace simd {

// [simd.iterator]
//
// simd-iterator is exposition-only, but its effects are observable: it is what makes basic_vec and
// basic_mask usable as ranges.
//
// Note the deliberate asymmetry between iterator_category and iterator_concept. operator* returns a
// prvalue value_type rather than a reference, so the type cannot satisfy the forward-iterator
// requirements even though it is random access in every other respect.
template <class _Vp>
class __simd_iterator {
  template <class>
  friend class __simd_iterator;

  _Vp* __data_               = nullptr;
  __simd_size_type __offset_ = 0;

public:
  using value_type        = typename _Vp::value_type;
  using iterator_category = input_iterator_tag;
  using iterator_concept  = random_access_iterator_tag;
  using difference_type   = __simd_size_type;

  _LIBCPP_HIDE_FROM_ABI constexpr __simd_iterator() = default;

  _LIBCPP_HIDE_FROM_ABI constexpr __simd_iterator(_Vp& __d, __simd_size_type __off) noexcept
      : __data_(std::addressof(__d)), __offset_(__off) {}

  _LIBCPP_HIDE_FROM_ABI constexpr __simd_iterator(const __simd_iterator&)            = default;
  _LIBCPP_HIDE_FROM_ABI constexpr __simd_iterator& operator=(const __simd_iterator&) = default;

  _LIBCPP_HIDE_FROM_ABI constexpr __simd_iterator(const __simd_iterator<remove_const_t<_Vp>>& __i)
    requires is_const_v<_Vp>
      : __data_(__i.__data_), __offset_(__i.__offset_) {}

  _LIBCPP_HIDE_FROM_ABI constexpr value_type operator*() const { return (*__data_)[__offset_]; }

  _LIBCPP_HIDE_FROM_ABI constexpr __simd_iterator& operator++() { return *this += 1; }

  _LIBCPP_HIDE_FROM_ABI constexpr __simd_iterator operator++(int) {
    __simd_iterator __tmp = *this;
    *this += 1;
    return __tmp;
  }

  _LIBCPP_HIDE_FROM_ABI constexpr __simd_iterator& operator--() { return *this -= 1; }

  _LIBCPP_HIDE_FROM_ABI constexpr __simd_iterator operator--(int) {
    __simd_iterator __tmp = *this;
    *this -= 1;
    return __tmp;
  }

  _LIBCPP_HIDE_FROM_ABI constexpr __simd_iterator& operator+=(difference_type __n) {
    __offset_ += __n;
    return *this;
  }

  _LIBCPP_HIDE_FROM_ABI constexpr __simd_iterator& operator-=(difference_type __n) {
    __offset_ -= __n;
    return *this;
  }

  _LIBCPP_HIDE_FROM_ABI constexpr value_type operator[](difference_type __n) const {
    return (*__data_)[__offset_ + __n];
  }

  _LIBCPP_HIDE_FROM_ABI friend constexpr bool operator==(__simd_iterator __a, __simd_iterator __b) = default;

  _LIBCPP_HIDE_FROM_ABI friend constexpr bool operator==(__simd_iterator __i, default_sentinel_t) noexcept {
    return __i.__offset_ == _Vp::size();
  }

  _LIBCPP_HIDE_FROM_ABI friend constexpr auto operator<=>(__simd_iterator __a, __simd_iterator __b) {
    return __a.__offset_ <=> __b.__offset_;
  }

  _LIBCPP_HIDE_FROM_ABI friend constexpr __simd_iterator operator+(__simd_iterator __i, difference_type __n) {
    return __i += __n;
  }

  _LIBCPP_HIDE_FROM_ABI friend constexpr __simd_iterator operator+(difference_type __n, __simd_iterator __i) {
    return __i += __n;
  }

  _LIBCPP_HIDE_FROM_ABI friend constexpr __simd_iterator operator-(__simd_iterator __i, difference_type __n) {
    return __i -= __n;
  }

  _LIBCPP_HIDE_FROM_ABI friend constexpr difference_type operator-(__simd_iterator __a, __simd_iterator __b) {
    return __a.__offset_ - __b.__offset_;
  }

  _LIBCPP_HIDE_FROM_ABI friend constexpr difference_type operator-(__simd_iterator __i, default_sentinel_t) noexcept {
    return __i.__offset_ - _Vp::size();
  }

  _LIBCPP_HIDE_FROM_ABI friend constexpr difference_type operator-(default_sentinel_t, __simd_iterator __i) noexcept {
    return _Vp::size() - __i.__offset_;
  }
};

} // namespace simd

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___SIMD_ITERATOR_H
