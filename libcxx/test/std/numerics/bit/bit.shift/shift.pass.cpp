//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <bit>

// template<class T, class S> constexpr T shl(T x, S s) noexcept;
// template<class T, class S> constexpr T shr(T x, S s) noexcept;

// P3793R2: x * 2^s (shl) and x * 2^-s (shr) rounded towards negative infinity, converted to T; no undefined behavior.

#include <bit>
#include <cassert>
#include <climits>
#include <concepts>
#include <cstdint>
#include <limits>

#include "test_macros.h"

#ifndef __cpp_lib_bitops
#  error __cpp_lib_bitops should be defined
#endif
#if __cpp_lib_bitops != 202607L
#  error __cpp_lib_bitops should have the value 202607L
#endif

template <class T, class S>
concept has_shl = requires(T x, S s) { std::shl(x, s); };

static_assert(!has_shl<bool, int>);
static_assert(!has_shl<int, bool>);
static_assert(!has_shl<char, int>);
static_assert(!has_shl<int, float>);
static_assert(!has_shl<float, int>);
static_assert(has_shl<signed char, unsigned long long>);

template <class T, class S>
constexpr bool test() {
  constexpr int N = std::numeric_limits<T>::digits + (std::is_signed_v<T> ? 1 : 0);
  static_assert(std::same_as<decltype(std::shl(T(1), S(1))), T>);
  static_assert(noexcept(std::shl(T(1), S(1))) && noexcept(std::shr(T(1), S(1))));

  // zero shift
  assert(std::shl(T(5), S(0)) == T(5));
  assert(std::shr(T(5), S(0)) == T(5));

  // in range
  assert(std::shl(T(1), S(3)) == T(8));
  assert(std::shr(T(64), S(3)) == T(8));
  assert(std::shl(T(3), S(2)) == T(12));

  // the shift count is the full width or more: no undefined behavior, the value is shifted out
  assert(std::shl(T(1), S(N - 1)) == static_cast<T>(static_cast<std::make_unsigned_t<T>>(1) << (N - 1)));
  assert(std::shl(T(1), S(N)) == T(0));
  assert(std::shl(T(1), S(N + 10)) == T(0));
  assert(std::shr(T(1), S(N)) == T(0));
  assert(std::shr(T(100), S(N + 10)) == T(0));
  assert(std::shl(T(1), std::numeric_limits<S>::max()) == T(0));
  assert(std::shr(T(1), std::numeric_limits<S>::max()) == T(0));

  if constexpr (std::is_signed_v<S>) {
    // a negative count shifts the other way, rounding towards negative infinity
    assert(std::shl(T(64), S(-3)) == T(8));
    assert(std::shr(T(1), S(-3)) == T(8));
    assert(std::shl(T(1), S(-1)) == T(0));
    assert(std::shr(T(1), S(-(N - 1))) == static_cast<T>(static_cast<std::make_unsigned_t<T>>(1) << (N - 1)));
    assert(std::shr(T(1), S(-N)) == T(0));
    assert(std::shl(T(1), std::numeric_limits<S>::min()) == T(0));
    assert(std::shr(T(1), std::numeric_limits<S>::min()) == T(0));
  }

  if constexpr (std::is_signed_v<T>) {
    // rounding towards negative infinity
    assert(std::shr(T(-1), S(1)) == T(-1));
    assert(std::shr(T(-5), S(1)) == T(-3));
    assert(std::shr(T(-8), S(2)) == T(-2));
    assert(std::shr(T(-1), S(N)) == T(-1));
    assert(std::shr(T(-1), S(N + 5)) == T(-1));
    assert(std::shl(T(-1), S(1)) == T(-2));
    assert(std::shl(T(-3), S(1)) == T(-6));
    assert(std::shl(std::numeric_limits<T>::min(), S(1)) == T(0));
    if constexpr (std::is_signed_v<S>) {
      assert(std::shl(T(-5), S(-1)) == T(-3));
      assert(std::shl(T(-1), S(-5)) == T(-1));
    }
    // the wrapped value is converted as if by static_cast<T>
    assert(std::shl(T(std::numeric_limits<T>::max()), S(1)) == T(-2));
  } else {
    assert(std::shl(std::numeric_limits<T>::max(), S(1)) == static_cast<T>(std::numeric_limits<T>::max() - 1));
  }
  return true;
}

template <class T>
constexpr bool test_all_shift_types() {
  test<T, signed char>();
  test<T, unsigned char>();
  test<T, short>();
  test<T, unsigned short>();
  test<T, int>();
  test<T, unsigned int>();
  test<T, long>();
  test<T, unsigned long>();
  test<T, long long>();
  test<T, unsigned long long>();
  return true;
}

constexpr bool test_all() {
  test_all_shift_types<signed char>();
  test_all_shift_types<unsigned char>();
  test_all_shift_types<short>();
  test_all_shift_types<unsigned short>();
  test_all_shift_types<int>();
  test_all_shift_types<unsigned int>();
  test_all_shift_types<long>();
  test_all_shift_types<unsigned long>();
  test_all_shift_types<long long>();
  test_all_shift_types<unsigned long long>();
#ifndef TEST_HAS_NO_INT128
  test<__int128_t, int>();
  test<__uint128_t, int>();
  test<int, __int128_t>();
#endif
  return true;
}

int main(int, char**) {
  test_all();
  static_assert(test_all());
  return 0;
}
