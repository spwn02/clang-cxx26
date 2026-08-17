// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___SIMD_BASIC_MASK_H
#define _LIBCPP___SIMD_BASIC_MASK_H

#include <__assert>
#include <__concepts/arithmetic.h>
#include <__concepts/same_as.h>
#include <__config>
#include <__cstddef/size_t.h>
#include <__simd/abi.h>
#include <__simd/concepts.h>
#include <__simd/iterator.h>
#include <__simd/traits.h>
#include <__simd/vectorizable.h>
#include <__type_traits/integral_constant.h>
#include <__utility/integer_sequence.h>
#include <bitset>
#include <limits>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

namespace simd {

// [simd.mask.ctor]/4: the generator constraint must hold for *every* index in [0, size()), and the
// result type must be exactly bool -- not merely convertible to it.
template <class _Gp, __simd_size_type _Np>
inline constexpr bool __mask_generator_ok = []<size_t... _Is>(index_sequence<_Is...>) consteval {
  return (requires(_Gp& __g) {
    { __g(integral_constant<__simd_size_type, static_cast<__simd_size_type>(_Is)>{}) } -> same_as<bool>;
  } && ...);
}(make_index_sequence<static_cast<size_t>(_Np < 0 ? 0 : _Np)>{});

// [simd.mask.overview]/1: a disabled basic_mask is a complete type with deleted default
// constructor, destructor, copy constructor and copy assignment, exposing only value_type and
// abi_type. It is emphatically not a hard error -- SFINAE probes depend on being able to name it.
template <size_t _Bytes, class _Abi>
class basic_mask {
public:
  using value_type = bool;
  using abi_type   = _Abi;

  basic_mask()                             = delete;
  ~basic_mask()                            = delete;
  basic_mask(const basic_mask&)            = delete;
  basic_mask& operator=(const basic_mask&) = delete;
};

template <size_t _Bytes, class _Abi>
  requires __mask_enabled<_Bytes, _Abi>
class basic_mask<_Bytes, _Abi> {
  static constexpr __simd_size_type __size_ = __mask_size_v<_Bytes, _Abi>;

  // A plain array with no initializer: default-initialization must leave the elements
  // default-initialized, and the type must stay trivially copyable.
  bool __storage_[static_cast<size_t>(__size_)];

  template <size_t, class>
  friend class basic_mask;
  template <class, class>
  friend class basic_vec;

public:
  using value_type     = bool;
  using abi_type       = _Abi;
  using iterator       = __simd_iterator<basic_mask>;
  using const_iterator = __simd_iterator<const basic_mask>;

  _LIBCPP_HIDE_FROM_ABI constexpr iterator begin() noexcept { return {*this, 0}; }
  _LIBCPP_HIDE_FROM_ABI constexpr const_iterator begin() const noexcept { return {*this, 0}; }
  _LIBCPP_HIDE_FROM_ABI constexpr const_iterator cbegin() const noexcept { return {*this, 0}; }
  _LIBCPP_HIDE_FROM_ABI constexpr default_sentinel_t end() const noexcept { return {}; }
  _LIBCPP_HIDE_FROM_ABI constexpr default_sentinel_t cend() const noexcept { return {}; }

  static constexpr integral_constant<__simd_size_type, __size_> size{};

  _LIBCPP_HIDE_FROM_ABI constexpr basic_mask() noexcept = default;

  // [simd.mask.ctor]/1 -- explicit, and only from bool exactly.
  _LIBCPP_HIDE_FROM_ABI constexpr explicit basic_mask(same_as<value_type> auto __x) noexcept {
    for (__simd_size_type __i = 0; __i != __size_; ++__i)
      __storage_[__i] = __x;
  }

  // [simd.mask.ctor]/2
  template <size_t _UBytes, class _UAbi>
    requires(__mask_enabled<_UBytes, _UAbi> && basic_mask<_UBytes, _UAbi>::size() == __size_)
  _LIBCPP_HIDE_FROM_ABI constexpr explicit basic_mask(const basic_mask<_UBytes, _UAbi>& __x) noexcept {
    for (__simd_size_type __i = 0; __i != __size_; ++__i)
      __storage_[__i] = __x[__i];
  }

  // [simd.mask.ctor]/4-6
  template <class _Gp>
    requires __mask_generator_ok<_Gp, __size_>
  _LIBCPP_HIDE_FROM_ABI constexpr explicit basic_mask(_Gp&& __gen) {
    [&]<size_t... _Is>(index_sequence<_Is...>) {
      // Comma fold: exactly one invocation per index, in increasing index order.
      ((__storage_[_Is] = __gen(integral_constant<__simd_size_type, static_cast<__simd_size_type>(_Is)>{})), ...);
    }(make_index_sequence<static_cast<size_t>(__size_)>{});
  }

  // [simd.mask.ctor]/7 -- implicit, unlike the others.
  template <same_as<bitset<static_cast<size_t>(__size_)>> _Tp>
  _LIBCPP_HIDE_FROM_ABI constexpr basic_mask(const _Tp& __b) noexcept {
    for (__simd_size_type __i = 0; __i != __size_; ++__i)
      __storage_[__i] = __b[static_cast<size_t>(__i)];
  }

  // [simd.mask.ctor]/8
  template <unsigned_integral _Tp>
    requires(!same_as<_Tp, value_type>)
  _LIBCPP_HIDE_FROM_ABI constexpr explicit basic_mask(_Tp __val) noexcept {
    constexpr __simd_size_type __bits = static_cast<__simd_size_type>(numeric_limits<_Tp>::digits);
    constexpr __simd_size_type __m    = __bits < __size_ ? __bits : __size_;
    for (__simd_size_type __i = 0; __i != __m; ++__i)
      __storage_[__i] = ((__val >> __i) & _Tp{1}) != _Tp{0};
    for (__simd_size_type __i = __m; __i != __size_; ++__i)
      __storage_[__i] = false;
  }

  // [simd.mask.subscr]
  _LIBCPP_HIDE_FROM_ABI constexpr value_type operator[](__simd_size_type __i) const {
    _LIBCPP_ASSERT_VALID_ELEMENT_ACCESS(
        __i >= 0 && __i < __size_, "simd::basic_mask::operator[] index is out of bounds");
    return __storage_[__i];
  }

  template <class _Ip>
    requires __simd_integral<_Ip>
  _LIBCPP_HIDE_FROM_ABI constexpr resize_t<_Ip::size(), basic_mask> operator[](const _Ip& __indices) const {
    return permute(*this, __indices);
  }

  // [simd.mask.unary]
  _LIBCPP_HIDE_FROM_ABI constexpr basic_mask operator!() const noexcept {
    return basic_mask([this](auto __i) -> bool { return !__storage_[__i]; });
  }

  // [simd.mask.unary]/3: these promote the mask to a vector of the signed integer type whose size
  // matches Bytes, and are deleted when no such vectorizable type exists (Bytes == 16).
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator+() const noexcept
    requires __has_integer_from<_Bytes>
  {
    return __to_vec_impl<__integer_from<_Bytes>>([](auto __v) { return +__v; });
  }
  _LIBCPP_HIDE_FROM_ABI constexpr void operator+() const noexcept
    requires(!__has_integer_from<_Bytes>)
  = delete;

  _LIBCPP_HIDE_FROM_ABI constexpr auto operator-() const noexcept
    requires __has_integer_from<_Bytes>
  {
    return __to_vec_impl<__integer_from<_Bytes>>([](auto __v) { return -__v; });
  }
  _LIBCPP_HIDE_FROM_ABI constexpr void operator-() const noexcept
    requires(!__has_integer_from<_Bytes>)
  = delete;

  _LIBCPP_HIDE_FROM_ABI constexpr auto operator~() const noexcept
    requires __has_integer_from<_Bytes>
  {
    return __to_vec_impl<__integer_from<_Bytes>>([](auto __v) { return static_cast<__integer_from<_Bytes>>(~__v); });
  }
  _LIBCPP_HIDE_FROM_ABI constexpr void operator~() const noexcept
    requires(!__has_integer_from<_Bytes>)
  = delete;

  // [simd.mask.conv]
  template <class _Up, class _Ap>
    requires(__vec_enabled<_Up, _Ap> && __simd_size_v<_Up, _Ap> == __size_)
  _LIBCPP_HIDE_FROM_ABI constexpr explicit(sizeof(_Up) != _Bytes) operator basic_vec<_Up, _Ap>() const noexcept {
    return basic_vec<_Up, _Ap>([this](auto __i) { return static_cast<_Up>(__storage_[__i]); });
  }

  _LIBCPP_HIDE_FROM_ABI constexpr bitset<static_cast<size_t>(__size_)> to_bitset() const noexcept {
    bitset<static_cast<size_t>(__size_)> __result;
    for (__simd_size_type __i = 0; __i != __size_; ++__i)
      __result[static_cast<size_t>(__i)] = __storage_[__i];
    return __result;
  }

  _LIBCPP_HIDE_FROM_ABI constexpr unsigned long long to_ullong() const {
    constexpr __simd_size_type __n = static_cast<__simd_size_type>(numeric_limits<unsigned long long>::digits);
    unsigned long long __result    = 0;
    const __simd_size_type __m     = __size_ < __n ? __size_ : __n;
    for (__simd_size_type __i = 0; __i != __m; ++__i)
      if (__storage_[__i])
        __result |= 1ULL << __i;
    return __result;
  }

  // [simd.mask.binary]
  _LIBCPP_HIDE_FROM_ABI friend constexpr basic_mask operator&&(const basic_mask& __a, const basic_mask& __b) noexcept {
    return basic_mask([&](auto __i) -> bool { return __a[__i] && __b[__i]; });
  }
  _LIBCPP_HIDE_FROM_ABI friend constexpr basic_mask operator||(const basic_mask& __a, const basic_mask& __b) noexcept {
    return basic_mask([&](auto __i) -> bool { return __a[__i] || __b[__i]; });
  }
  _LIBCPP_HIDE_FROM_ABI friend constexpr basic_mask operator&(const basic_mask& __a, const basic_mask& __b) noexcept {
    return basic_mask([&](auto __i) -> bool { return __a[__i] && __b[__i]; });
  }
  _LIBCPP_HIDE_FROM_ABI friend constexpr basic_mask operator|(const basic_mask& __a, const basic_mask& __b) noexcept {
    return basic_mask([&](auto __i) -> bool { return __a[__i] || __b[__i]; });
  }
  _LIBCPP_HIDE_FROM_ABI friend constexpr basic_mask operator^(const basic_mask& __a, const basic_mask& __b) noexcept {
    return basic_mask([&](auto __i) -> bool { return __a[__i] != __b[__i]; });
  }

  // [simd.mask.cassign]
  _LIBCPP_HIDE_FROM_ABI friend constexpr basic_mask& operator&=(basic_mask& __a, const basic_mask& __b) noexcept {
    return __a = __a & __b;
  }
  _LIBCPP_HIDE_FROM_ABI friend constexpr basic_mask& operator|=(basic_mask& __a, const basic_mask& __b) noexcept {
    return __a = __a | __b;
  }
  _LIBCPP_HIDE_FROM_ABI friend constexpr basic_mask& operator^=(basic_mask& __a, const basic_mask& __b) noexcept {
    return __a = __a ^ __b;
  }

  // [simd.mask.comparison] -- note these return basic_mask, not bool.
  _LIBCPP_HIDE_FROM_ABI friend constexpr basic_mask operator==(const basic_mask& __a, const basic_mask& __b) noexcept {
    return basic_mask([&](auto __i) -> bool { return __a[__i] == __b[__i]; });
  }
  _LIBCPP_HIDE_FROM_ABI friend constexpr basic_mask operator!=(const basic_mask& __a, const basic_mask& __b) noexcept {
    return basic_mask([&](auto __i) -> bool { return __a[__i] != __b[__i]; });
  }
  _LIBCPP_HIDE_FROM_ABI friend constexpr basic_mask operator>=(const basic_mask& __a, const basic_mask& __b) noexcept {
    return basic_mask([&](auto __i) -> bool { return __a[__i] >= __b[__i]; });
  }
  _LIBCPP_HIDE_FROM_ABI friend constexpr basic_mask operator<=(const basic_mask& __a, const basic_mask& __b) noexcept {
    return basic_mask([&](auto __i) -> bool { return __a[__i] <= __b[__i]; });
  }
  _LIBCPP_HIDE_FROM_ABI friend constexpr basic_mask operator>(const basic_mask& __a, const basic_mask& __b) noexcept {
    return basic_mask([&](auto __i) -> bool { return __a[__i] > __b[__i]; });
  }
  _LIBCPP_HIDE_FROM_ABI friend constexpr basic_mask operator<(const basic_mask& __a, const basic_mask& __b) noexcept {
    return basic_mask([&](auto __i) -> bool { return __a[__i] < __b[__i]; });
  }

  // [simd.mask.cond] -- found by ADL only, which is what lets select() deduce its return type.
  _LIBCPP_HIDE_FROM_ABI friend constexpr basic_mask
  __simd_select_impl(const basic_mask& __k, const basic_mask& __a, const basic_mask& __b) noexcept {
    return basic_mask([&](auto __i) -> bool { return __k[__i] ? __a[__i] : __b[__i]; });
  }

  _LIBCPP_HIDE_FROM_ABI friend constexpr basic_mask
  __simd_select_impl(const basic_mask& __k, same_as<bool> auto __a, same_as<bool> auto __b) noexcept {
    return basic_mask([&](auto __i) -> bool { return __k[__i] ? __a : __b; });
  }

  template <class _T0, class _T1>
    requires(same_as<_T0, _T1> && __vectorizable<_T0> && sizeof(_T0) == _Bytes)
  _LIBCPP_HIDE_FROM_ABI friend constexpr vec<_T0, __size_>
  __simd_select_impl(const basic_mask& __k, const _T0& __a, const _T1& __b) noexcept {
    return vec<_T0, __size_>([&](auto __i) { return __k[__i] ? __a : __b; });
  }

private:
  template <class _Ip, class _Fn>
  _LIBCPP_HIDE_FROM_ABI constexpr auto __to_vec_impl(_Fn __fn) const noexcept {
    return basic_vec<_Ip, _Abi>([&](auto __i) { return __fn(static_cast<_Ip>(__storage_[__i])); });
  }
};

} // namespace simd

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___SIMD_BASIC_MASK_H
