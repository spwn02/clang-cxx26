//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef SUPPORT_INTEGER_CLASS_H
#define SUPPORT_INTEGER_CLASS_H

// Integer-class types in the sense of [iterator.concept.winc] (P2393R1): 128-bit two's complement signed and unsigned
// class types that behave as integers do. They are wider than every built-in integer type of libc++ except
// __int128, which they deliberately do not build on for their interface: conversions from built-in integral types
// are implicit, conversions to them and to bool are explicit, and every arithmetic, bitwise, shift, comparison and
// increment operator is provided. numeric_limits is specialized, as the paper requires, and the types name their
// counterpart of the other signedness the way libc++'s make-signed-like-t / make-unsigned-like-t look for it.

#include <compare>
#include <concepts>
#include <cstdint>
#include <limits>
#include <type_traits>

#include "test_macros.h"

#ifdef __SIZEOF_INT128__

namespace integer_class {

template <bool IsSigned>
class Int128;

using Signed  = Int128<true>;
using Unsigned = Int128<false>;

template <bool IsSigned>
class Int128 {
  using Rep = std::conditional_t<IsSigned, __int128, unsigned __int128>;
  Rep value_;

  constexpr explicit Int128(Rep v, int) noexcept : value_(v) {}

public:
  using signed_type   = Int128<true>;
  using unsigned_type = Int128<false>;
  // libc++'s difference_type / size_type of ranges built on these types
  static constexpr bool is_signed_class = IsSigned;

  constexpr Int128() noexcept : value_(0) {}
  template <std::integral I>
  constexpr Int128(I v) noexcept : value_(static_cast<Rep>(v)) {}
  template <bool S>
  constexpr explicit Int128(Int128<S> other) noexcept : value_(static_cast<Rep>(other.value_)) {}

  template <std::integral I>
  constexpr explicit operator I() const noexcept {
    return static_cast<I>(value_);
  }
  constexpr explicit operator bool() const noexcept { return value_ != 0; }

  // unary
  constexpr Int128 operator+() const noexcept { return *this; }
  constexpr Int128 operator-() const noexcept { return Int128(static_cast<Rep>(Rep(0) - value_), 0); }
  constexpr Int128 operator~() const noexcept { return Int128(static_cast<Rep>(~value_), 0); }
  constexpr bool operator!() const noexcept { return value_ == 0; }

  // increment / decrement
  constexpr Int128& operator++() noexcept { ++value_; return *this; }
  constexpr Int128 operator++(int) noexcept { Int128 t = *this; ++value_; return t; }
  constexpr Int128& operator--() noexcept { --value_; return *this; }
  constexpr Int128 operator--(int) noexcept { Int128 t = *this; --value_; return t; }

  // compound assignment (two's complement wrap-around for the unsigned type; the signed type wraps via unsigned)
  constexpr Int128& operator+=(Int128 o) noexcept { value_ = static_cast<Rep>(static_cast<unsigned __int128>(value_) + static_cast<unsigned __int128>(o.value_)); return *this; }
  constexpr Int128& operator-=(Int128 o) noexcept { value_ = static_cast<Rep>(static_cast<unsigned __int128>(value_) - static_cast<unsigned __int128>(o.value_)); return *this; }
  constexpr Int128& operator*=(Int128 o) noexcept { value_ = static_cast<Rep>(static_cast<unsigned __int128>(value_) * static_cast<unsigned __int128>(o.value_)); return *this; }
  constexpr Int128& operator/=(Int128 o) noexcept { value_ /= o.value_; return *this; }
  constexpr Int128& operator%=(Int128 o) noexcept { value_ %= o.value_; return *this; }
  constexpr Int128& operator&=(Int128 o) noexcept { value_ &= o.value_; return *this; }
  constexpr Int128& operator|=(Int128 o) noexcept { value_ |= o.value_; return *this; }
  constexpr Int128& operator^=(Int128 o) noexcept { value_ ^= o.value_; return *this; }
  template <std::integral I>
  constexpr Int128& operator<<=(I n) noexcept { value_ = static_cast<Rep>(static_cast<unsigned __int128>(value_) << n); return *this; }
  template <std::integral I>
  constexpr Int128& operator>>=(I n) noexcept { value_ >>= n; return *this; }

  // binary
  friend constexpr Int128 operator+(Int128 a, Int128 b) noexcept { return a += b; }
  friend constexpr Int128 operator-(Int128 a, Int128 b) noexcept { return a -= b; }
  friend constexpr Int128 operator*(Int128 a, Int128 b) noexcept { return a *= b; }
  friend constexpr Int128 operator/(Int128 a, Int128 b) noexcept { return a /= b; }
  friend constexpr Int128 operator%(Int128 a, Int128 b) noexcept { return a %= b; }
  friend constexpr Int128 operator&(Int128 a, Int128 b) noexcept { return a &= b; }
  friend constexpr Int128 operator|(Int128 a, Int128 b) noexcept { return a |= b; }
  friend constexpr Int128 operator^(Int128 a, Int128 b) noexcept { return a ^= b; }
  template <std::integral I>
  friend constexpr Int128 operator<<(Int128 a, I n) noexcept { return a <<= n; }
  template <std::integral I>
  friend constexpr Int128 operator>>(Int128 a, I n) noexcept { return a >>= n; }

  friend constexpr bool operator==(Int128 a, Int128 b) noexcept { return a.value_ == b.value_; }
  friend constexpr std::strong_ordering operator<=>(Int128 a, Int128 b) noexcept { return a.value_ <=> b.value_; }

  template <bool S>
  friend class Int128;
  friend struct std::numeric_limits<Int128>;
};

} // namespace integer_class

template <bool S>
struct std::numeric_limits<integer_class::Int128<S>> {
  using T = integer_class::Int128<S>;
  static constexpr bool is_specialized = true;
  static constexpr bool is_signed      = S;
  static constexpr bool is_integer     = true;
  static constexpr bool is_exact       = true;
  static constexpr bool is_bounded     = true;
  static constexpr bool is_modulo      = !S;
  static constexpr int digits          = S ? 127 : 128;
  static constexpr int radix           = 2;
  static constexpr T min() noexcept {
    if constexpr (S)
      return T(static_cast<typename T::Rep>(static_cast<unsigned __int128>(1) << 127), 0);
    else
      return T(0);
  }
  static constexpr T max() noexcept {
    if constexpr (S)
      return T(static_cast<typename T::Rep>(~(static_cast<unsigned __int128>(1) << 127)), 0);
    else
      return T(static_cast<typename T::Rep>(~static_cast<unsigned __int128>(0)), 0);
  }
  static constexpr T lowest() noexcept { return min(); }
};

#endif // __SIZEOF_INT128__

#endif // SUPPORT_INTEGER_CLASS_H
