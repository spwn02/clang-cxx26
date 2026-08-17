// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___SIMD_BASIC_VEC_H
#define _LIBCPP___SIMD_BASIC_VEC_H

#include <__assert>
#include <__concepts/arithmetic.h>
#include <__concepts/convertible_to.h>
#include <__concepts/equality_comparable.h>
#include <__concepts/same_as.h>
#include <__config>
#include <__cstddef/size_t.h>
#include <__ranges/concepts.h>
#include <__ranges/data.h>
#include <__ranges/size.h>
#include <__simd/abi.h>
#include <__simd/basic_mask.h>
#include <__simd/concepts.h>
#include <__simd/flags.h>
#include <__simd/iterator.h>
#include <__simd/traits.h>
#include <__simd/vectorizable.h>
#include <__type_traits/integral_constant.h>
#include <__type_traits/is_arithmetic.h>
#include <__type_traits/is_same.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/integer_sequence.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

namespace simd {

// The constexpr-wrapper-like concept from [expr.const], reproduced here because <simd> is the only
// consumer in libc++ so far. It is what lets basic_vec(integral_constant<int, 3>{}) work.
template <class _Tp>
concept __constexpr_wrapper_like =
    convertible_to<_Tp, const decltype(_Tp::value)&> && equality_comparable_with<_Tp, decltype(_Tp::value)> &&
    bool_constant<_Tp() == _Tp::value>::value &&
    bool_constant<static_cast<decltype(_Tp::value)>(_Tp()) == _Tp::value>::value;

// [simd.ctor]/6: the converting constructor is explicit when the conversion loses information or
// when the source has the greater conversion rank. Rank is deliberately not sizeof: long and long
// long are the same size on LP64 but have different ranks, and that case must be explicit.
template <class _From, class _To>
inline constexpr bool __vec_conversion_is_explicit = [] consteval {
  if constexpr (is_same_v<_From, _To>) {
    return false;
  } else if constexpr (is_arithmetic_v<_From> && is_arithmetic_v<_To>) {
    if (!__value_preserving_conversion<_From, _To>)
      return true;
    if (integral<_From> && integral<_To>)
      return __integer_conversion_rank<_From> > __integer_conversion_rank<_To>;
    if (floating_point<_From> && floating_point<_To>)
      return __floating_point_conversion_rank<_From> > __floating_point_conversion_rank<_To>;
    return false;
  } else {
    return true;
  }
}();

// [simd.ctor]/8: the generator constraint must hold for every index, with per-index
// value-preservation for arithmetic results -- not merely at index 0.
template <class _Gp, class _Tp, __simd_size_type _Np>
inline constexpr bool __vec_generator_ok = []<size_t... _Is>(index_sequence<_Is...>) consteval {
  auto __one = []<size_t _Idx>(integral_constant<size_t, _Idx>) consteval {
    using _Ic = integral_constant<__simd_size_type, static_cast<__simd_size_type>(_Idx)>;
    if constexpr (!requires(_Gp& __g) { __g(_Ic{}); }) {
      return false;
    } else {
      using _From = remove_cvref_t<decltype(std::declval<_Gp&>()(_Ic{}))>;
      if constexpr (!convertible_to<_From, _Tp>)
        return false;
      else if constexpr (is_arithmetic_v<_From>)
        return __value_preserving_conversion<_From, _Tp>;
      else
        return true;
    }
  };
  return (__one(integral_constant<size_t, _Is>{}) && ...);
}(make_index_sequence<static_cast<size_t>(_Np < 0 ? 0 : _Np)>{});

// [simd.overview]/3: real-type is rebind_t<typename T::value_type, basic_vec<T, Abi>> when T is a
// specialization of complex, and an unspecified non-array object type otherwise.
struct __no_real_type {};

template <class _Tp, class _Abi>
struct __real_type_of {
  using type _LIBCPP_NODEBUG = __no_real_type;
};

template <class _Tp, class _Abi>
  requires __vectorizable_complex<_Tp>
struct __real_type_of<_Tp, _Abi> {
  using type _LIBCPP_NODEBUG = basic_vec<typename _Tp::value_type, _Abi>;
};

// [simd.overview]/1: a disabled basic_vec is a complete type with deleted default constructor,
// destructor, copy constructor and copy assignment, exposing only value_type, abi_type and
// mask_type. Making this a static_assert instead would break every SFINAE probe.
template <class _Tp, class _Abi>
class basic_vec {
public:
  using value_type = _Tp;
  using abi_type   = _Abi;
  using mask_type  = basic_mask<sizeof(_Tp), _Abi>;

  basic_vec()                            = delete;
  ~basic_vec()                           = delete;
  basic_vec(const basic_vec&)            = delete;
  basic_vec& operator=(const basic_vec&) = delete;
};

template <class _Tp, class _Abi>
  requires __vec_enabled<_Tp, _Abi>
class basic_vec<_Tp, _Abi> {
  static constexpr __simd_size_type __size_ = __simd_size_v<_Tp, _Abi>;

  // No initializer: [simd.overview]/1.5 requires default-initialization to default-initialize the
  // elements, and the type must remain trivially copyable.
  _Tp __storage_[static_cast<size_t>(__size_)];

  template <class, class>
  friend class basic_vec;
  template <size_t, class>
  friend class basic_mask;

  using __real_type _LIBCPP_NODEBUG = typename __real_type_of<_Tp, _Abi>::type;

public:
  using value_type     = _Tp;
  using abi_type       = _Abi;
  using mask_type      = basic_mask<sizeof(_Tp), _Abi>;
  using iterator       = __simd_iterator<basic_vec>;
  using const_iterator = __simd_iterator<const basic_vec>;

  _LIBCPP_HIDE_FROM_ABI constexpr iterator begin() noexcept { return {*this, 0}; }
  _LIBCPP_HIDE_FROM_ABI constexpr const_iterator begin() const noexcept { return {*this, 0}; }
  _LIBCPP_HIDE_FROM_ABI constexpr const_iterator cbegin() const noexcept { return {*this, 0}; }
  _LIBCPP_HIDE_FROM_ABI constexpr default_sentinel_t end() const noexcept { return {}; }
  _LIBCPP_HIDE_FROM_ABI constexpr default_sentinel_t cend() const noexcept { return {}; }

  static constexpr integral_constant<__simd_size_type, __size_> size{};

  _LIBCPP_HIDE_FROM_ABI constexpr basic_vec() noexcept = default;

  // [simd.ctor]/1-3 -- broadcast. Implicit by design: the mixed vec/scalar math overloads rely on
  // it, which is exactly why the value-preserving constraint matters. vec<int, 4>{3.7} must not
  // compile.
  template <class _Up>
    requires((convertible_to<_Up, value_type> && !is_arithmetic_v<remove_cvref_t<_Up>> &&
              !__constexpr_wrapper_like<remove_cvref_t<_Up>>) ||
             (is_arithmetic_v<remove_cvref_t<_Up>> && __value_preserving_conversion<remove_cvref_t<_Up>, _Tp>) ||
             (__constexpr_wrapper_like<remove_cvref_t<_Up>> &&
              is_arithmetic_v<remove_cvref_t<decltype(remove_cvref_t<_Up>::value)>> &&
              __value_preserving_conversion<remove_cvref_t<decltype(remove_cvref_t<_Up>::value)>, _Tp>))
  _LIBCPP_HIDE_FROM_ABI constexpr basic_vec(_Up&& __value) noexcept {
    const auto __converted = static_cast<_Tp>(__value);
    for (__simd_size_type __i = 0; __i != __size_; ++__i)
      __storage_[__i] = __converted;
  }

  // [simd.ctor]/4-6
  template <class _Up, class _UAbi>
    requires(__vec_enabled<_Up, _UAbi> && __simd_size_v<_Up, _UAbi> == __size_ &&
             __explicitly_convertible_to<_Up, _Tp>)
  _LIBCPP_HIDE_FROM_ABI constexpr explicit(__vec_conversion_is_explicit<_Up, _Tp>)
      basic_vec(const basic_vec<_Up, _UAbi>& __x) noexcept {
    for (__simd_size_type __i = 0; __i != __size_; ++__i)
      __storage_[__i] = static_cast<_Tp>(__x[__i]);
  }

  // [simd.ctor]/7-10
  template <class _Gp>
    requires __vec_generator_ok<_Gp, _Tp, __size_>
  _LIBCPP_HIDE_FROM_ABI constexpr explicit basic_vec(_Gp&& __gen) {
    [&]<size_t... _Is>(index_sequence<_Is...>) {
      // Comma fold: exactly one invocation per index, in increasing index order.
      ((__storage_[_Is] =
            static_cast<_Tp>(__gen(integral_constant<__simd_size_type, static_cast<__simd_size_type>(_Is)>{}))),
       ...);
    }(make_index_sequence<static_cast<size_t>(__size_)>{});
  }

  // [simd.ctor]/11-15 -- range construction.
  //
  // KNOWN DEVIATION: [simd.ctor]/12.2 additionally Constrains these on ranges::size(r) being a
  // constant expression, and /12.3 on it being equal to size(). Neither is expressible without a
  // usable constant-evaluable range object, so the size equality is enforced as a precondition
  // below instead of as a constraint. See the handoff notes.
  template <class _Rp, class... _Flags>
    requires(ranges::contiguous_range<_Rp> && ranges::sized_range<_Rp> &&
             __vectorizable<ranges::range_value_t<_Rp>> && __explicitly_convertible_to<ranges::range_value_t<_Rp>, _Tp>)
  _LIBCPP_HIDE_FROM_ABI constexpr basic_vec(_Rp&& __r, flags<_Flags...> = {}) {
    static_assert(__flags_have_convert<_Flags...> || __value_preserving_conversion<ranges::range_value_t<_Rp>, _Tp>,
                  "simd::basic_vec range constructor: the conversion from the range's value type to "
                  "value_type is not value-preserving; pass simd::flag_convert to allow it");
    _LIBCPP_ASSERT_VALID_INPUT_RANGE(static_cast<__simd_size_type>(ranges::size(__r)) == __size_,
                                     "simd::basic_vec range constructor: ranges::size(r) must equal size()");
    auto* __p = ranges::data(__r);
    for (__simd_size_type __i = 0; __i != __size_; ++__i)
      __storage_[__i] = static_cast<_Tp>(__p[__i]);
  }

  template <class _Rp, class... _Flags>
    requires(ranges::contiguous_range<_Rp> && ranges::sized_range<_Rp> &&
             __vectorizable<ranges::range_value_t<_Rp>> && __explicitly_convertible_to<ranges::range_value_t<_Rp>, _Tp>)
  _LIBCPP_HIDE_FROM_ABI constexpr basic_vec(_Rp&& __r, const mask_type& __mask, flags<_Flags...> = {}) {
    static_assert(__flags_have_convert<_Flags...> || __value_preserving_conversion<ranges::range_value_t<_Rp>, _Tp>,
                  "simd::basic_vec range constructor: the conversion from the range's value type to "
                  "value_type is not value-preserving; pass simd::flag_convert to allow it");
    _LIBCPP_ASSERT_VALID_INPUT_RANGE(static_cast<__simd_size_type>(ranges::size(__r)) == __size_,
                                     "simd::basic_vec range constructor: ranges::size(r) must equal size()");
    auto* __p = ranges::data(__r);
    for (__simd_size_type __i = 0; __i != __size_; ++__i)
      __storage_[__i] = __mask[__i] ? static_cast<_Tp>(__p[__i]) : _Tp();
  }

  // [simd.ctor]/20-21 -- complex construction from separate real and imaginary parts.
  _LIBCPP_HIDE_FROM_ABI constexpr basic_vec(const __real_type& __reals, const __real_type& __imags = {}) noexcept
    requires __vectorizable_complex<_Tp>
  {
    for (__simd_size_type __i = 0; __i != __size_; ++__i)
      __storage_[__i] = _Tp(__reals[__i], __imags[__i]);
  }

  // [simd.subscr]
  _LIBCPP_HIDE_FROM_ABI constexpr value_type operator[](__simd_size_type __i) const {
    _LIBCPP_ASSERT_VALID_ELEMENT_ACCESS(
        __i >= 0 && __i < __size_, "simd::basic_vec::operator[] index is out of bounds");
    return __storage_[__i];
  }

  template <class _Ip>
    requires __simd_integral<_Ip>
  _LIBCPP_HIDE_FROM_ABI constexpr resize_t<_Ip::size(), basic_vec> operator[](const _Ip& __indices) const {
    return permute(*this, __indices);
  }

  // [simd.complex.access]
  _LIBCPP_HIDE_FROM_ABI constexpr __real_type real() const noexcept
    requires __vectorizable_complex<_Tp>
  {
    return __real_type([this](auto __i) { return __storage_[__i].real(); });
  }

  _LIBCPP_HIDE_FROM_ABI constexpr __real_type imag() const noexcept
    requires __vectorizable_complex<_Tp>
  {
    return __real_type([this](auto __i) { return __storage_[__i].imag(); });
  }

  _LIBCPP_HIDE_FROM_ABI constexpr void real(const __real_type& __v) noexcept
    requires __vectorizable_complex<_Tp>
  {
    for (__simd_size_type __i = 0; __i != __size_; ++__i)
      __storage_[__i] = _Tp(__v[__i], __storage_[__i].imag());
  }

  _LIBCPP_HIDE_FROM_ABI constexpr void imag(const __real_type& __v) noexcept
    requires __vectorizable_complex<_Tp>
  {
    for (__simd_size_type __i = 0; __i != __size_; ++__i)
      __storage_[__i] = _Tp(__storage_[__i].real(), __v[__i]);
  }

  // [simd.unary] -- each operator is constrained on the element operation being valid, so that
  // e.g. `requires { ~v; }` correctly answers false for vec<float>.
  _LIBCPP_HIDE_FROM_ABI constexpr basic_vec& operator++() noexcept
    requires requires(value_type __a) { ++__a; }
  {
    for (__simd_size_type __i = 0; __i != __size_; ++__i)
      ++__storage_[__i];
    return *this;
  }

  _LIBCPP_HIDE_FROM_ABI constexpr basic_vec operator++(int) noexcept
    requires requires(value_type __a) { __a++; }
  {
    basic_vec __tmp = *this;
    for (__simd_size_type __i = 0; __i != __size_; ++__i)
      __storage_[__i]++;
    return __tmp;
  }

  _LIBCPP_HIDE_FROM_ABI constexpr basic_vec& operator--() noexcept
    requires requires(value_type __a) { --__a; }
  {
    for (__simd_size_type __i = 0; __i != __size_; ++__i)
      --__storage_[__i];
    return *this;
  }

  _LIBCPP_HIDE_FROM_ABI constexpr basic_vec operator--(int) noexcept
    requires requires(value_type __a) { __a--; }
  {
    basic_vec __tmp = *this;
    for (__simd_size_type __i = 0; __i != __size_; ++__i)
      __storage_[__i]--;
    return __tmp;
  }

  _LIBCPP_HIDE_FROM_ABI constexpr mask_type operator!() const noexcept
    requires requires(const value_type __a) { !__a; }
  {
    return mask_type([this](auto __i) -> bool { return !__storage_[__i]; });
  }

  _LIBCPP_HIDE_FROM_ABI constexpr basic_vec operator~() const noexcept
    requires requires(const value_type __a) { ~__a; }
  {
    return basic_vec([this](auto __i) { return static_cast<_Tp>(~__storage_[__i]); });
  }

  _LIBCPP_HIDE_FROM_ABI constexpr basic_vec operator+() const noexcept
    requires requires(const value_type __a) { +__a; }
  {
    return *this;
  }

  _LIBCPP_HIDE_FROM_ABI constexpr basic_vec operator-() const noexcept
    requires requires(const value_type __a) { -__a; }
  {
    return basic_vec([this](auto __i) { return static_cast<_Tp>(-__storage_[__i]); });
  }

  // [simd.binary] / [simd.cassign] / [simd.comparison]
#  define _LIBCPP_SIMD_BINARY_OP(_Op)                                                                                  \
    _LIBCPP_HIDE_FROM_ABI friend constexpr basic_vec operator _Op(const basic_vec& __a, const basic_vec& __b) noexcept  \
      requires requires(value_type __x, value_type __y) { __x _Op __y; }                                               \
    {                                                                                                                  \
      return basic_vec([&](auto __i) { return static_cast<_Tp>(__a[__i] _Op __b[__i]); });                              \
    }                                                                                                                  \
    _LIBCPP_HIDE_FROM_ABI friend constexpr basic_vec& operator _Op##=(basic_vec& __a, const basic_vec& __b) noexcept    \
      requires requires(value_type __x, value_type __y) { __x _Op __y; }                                               \
    {                                                                                                                  \
      return __a = __a _Op __b;                                                                                        \
    }

  _LIBCPP_SIMD_BINARY_OP(+)
  _LIBCPP_SIMD_BINARY_OP(-)
  _LIBCPP_SIMD_BINARY_OP(*)
  _LIBCPP_SIMD_BINARY_OP(/)
  _LIBCPP_SIMD_BINARY_OP(%)
  _LIBCPP_SIMD_BINARY_OP(&)
  _LIBCPP_SIMD_BINARY_OP(|)
  _LIBCPP_SIMD_BINARY_OP(^)
  _LIBCPP_SIMD_BINARY_OP(<<)
  _LIBCPP_SIMD_BINARY_OP(>>)
#  undef _LIBCPP_SIMD_BINARY_OP

  // [simd.binary]/4-6 and [simd.cassign]/5-7 -- shift by a scalar. These are separate overloads,
  // not a use of the broadcast constructor: their constraint is on `a op simd-size-type`.
#  define _LIBCPP_SIMD_SHIFT_OP(_Op)                                                                                   \
    _LIBCPP_HIDE_FROM_ABI friend constexpr basic_vec operator _Op(const basic_vec& __v, __simd_size_type __n) noexcept  \
      requires requires(value_type __x, __simd_size_type __y) { __x _Op __y; }                                         \
    {                                                                                                                  \
      return basic_vec([&](auto __i) { return static_cast<_Tp>(__v[__i] _Op __n); });                                  \
    }                                                                                                                  \
    _LIBCPP_HIDE_FROM_ABI friend constexpr basic_vec& operator _Op##=(basic_vec& __v, __simd_size_type __n) noexcept    \
      requires requires(value_type __x, __simd_size_type __y) { __x _Op __y; }                                         \
    {                                                                                                                  \
      return __v = __v _Op __n;                                                                                        \
    }

  _LIBCPP_SIMD_SHIFT_OP(<<)
  _LIBCPP_SIMD_SHIFT_OP(>>)
#  undef _LIBCPP_SIMD_SHIFT_OP

#  define _LIBCPP_SIMD_COMPARE_OP(_Op)                                                                                 \
    _LIBCPP_HIDE_FROM_ABI friend constexpr mask_type operator _Op(const basic_vec& __a, const basic_vec& __b) noexcept  \
      requires requires(value_type __x, value_type __y) { __x _Op __y; }                                               \
    {                                                                                                                  \
      return mask_type([&](auto __i) -> bool { return __a[__i] _Op __b[__i]; });                                       \
    }

  _LIBCPP_SIMD_COMPARE_OP(==)
  _LIBCPP_SIMD_COMPARE_OP(!=)
  _LIBCPP_SIMD_COMPARE_OP(>=)
  _LIBCPP_SIMD_COMPARE_OP(<=)
  _LIBCPP_SIMD_COMPARE_OP(>)
  _LIBCPP_SIMD_COMPARE_OP(<)
#  undef _LIBCPP_SIMD_COMPARE_OP

  // [simd.cond] -- ADL-only, per [simd.alg]/10.
  _LIBCPP_HIDE_FROM_ABI friend constexpr basic_vec
  __simd_select_impl(const mask_type& __k, const basic_vec& __a, const basic_vec& __b) noexcept {
    return basic_vec([&](auto __i) { return __k[__i] ? __a[__i] : __b[__i]; });
  }
};

// [simd.syn]
template <class _Tp, __simd_size_type _Np = __simd_size_v<_Tp, __native_abi<_Tp>>>
using mask = typename vec<_Tp, _Np>::mask_type;

} // namespace simd

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___SIMD_BASIC_VEC_H
