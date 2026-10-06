//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// The reference implementations of the formulas are evaluated in constant expressions too.
// ADDITIONAL_COMPILE_FLAGS(has-fconstexpr-steps): -fconstexpr-steps=50000000

// <bit>

// template<class T> constexpr T bit_reverse(T x) noexcept;
// template<class T> constexpr T bit_repeat(T x, int l);
// template<class T> constexpr T bit_compress(T x, T m) noexcept;
// template<class T> constexpr T bit_expand(T x, T m) noexcept;

// [bit.permute]: T is an unsigned integer type; N is numeric_limits<T>::digits.

#include <bit>
#include <cassert>
#include <concepts>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <type_traits>

#include "test_macros.h"

// the formulas of [bit.permute], written out
template <class T>
constexpr int bit_of(T v, int n) {
  return static_cast<int>((v >> n) & 1);
}
template <class T>
constexpr int sigma(T m, int n) {
  int c = 0;
  for (int k = 0; k < n; ++k)
    c += bit_of(m, k);
  return c;
}
template <class T>
constexpr T reverse_ref(T x) {
  constexpr int N = std::numeric_limits<T>::digits;
  T r             = 0;
  for (int n = 0; n < N; ++n)
    r = static_cast<T>(r | (static_cast<T>(bit_of(x, n)) << (N - n - 1)));
  return r;
}
template <class T>
constexpr T repeat_ref(T x, int l) {
  constexpr int N = std::numeric_limits<T>::digits;
  T r             = 0;
  for (int n = 0; n < N; ++n)
    r = static_cast<T>(r | (static_cast<T>(bit_of(x, n % l)) << n));
  return r;
}
template <class T>
constexpr T compress_ref(T x, T m) {
  constexpr int N = std::numeric_limits<T>::digits;
  T r             = 0;
  for (int n = 0; n < N; ++n)
    if (bit_of(m, n))
      r = static_cast<T>(r | (static_cast<T>(bit_of(x, n)) << sigma(m, n)));
  return r;
}
template <class T>
constexpr T expand_ref(T x, T m) {
  constexpr int N = std::numeric_limits<T>::digits;
  T r             = 0;
  for (int n = 0; n < N; ++n)
    if (bit_of(m, n))
      r = static_cast<T>(r | (static_cast<T>(bit_of(x, sigma(m, n))) << n));
  return r;
}

template <class T>
constexpr void test_type() {
  ASSERT_SAME_TYPE(decltype(std::bit_reverse(T{})), T);
  ASSERT_SAME_TYPE(decltype(std::bit_repeat(T{}, 1)), T);
  ASSERT_SAME_TYPE(decltype(std::bit_compress(T{}, T{})), T);
  ASSERT_SAME_TYPE(decltype(std::bit_expand(T{}, T{})), T);
  ASSERT_NOEXCEPT(std::bit_reverse(T{}));
  ASSERT_NOEXCEPT(std::bit_compress(T{}, T{}));
  ASSERT_NOEXCEPT(std::bit_expand(T{}, T{}));

  constexpr int N = std::numeric_limits<T>::digits;
  const T all     = std::numeric_limits<T>::max();
  const T samples[] = {T(0),
                       T(1),
                       T(2),
                       T(3),
                       T(0x5a),
                       T(0xa5),
                       T(0x96),
                       static_cast<T>(all - 1),
                       all,
                       static_cast<T>(T(1) << (N - 1)),
                       static_cast<T>(all / 3),
                       static_cast<T>(all / 5 * 2)};

  for (T x : samples) {
    assert(std::bit_reverse(x) == reverse_ref(x));
    assert(std::bit_reverse(std::bit_reverse(x)) == x);
    for (int l : {1, 2, 3, 4, 7, N - 1, N, N + 1, 1000})
      assert(std::bit_repeat(x, l) == repeat_ref(x, l >= N ? N : l));
    for (T m : samples) {
      assert(std::bit_compress(x, m) == compress_ref(x, m));
      assert(std::bit_expand(x, m) == expand_ref(x, m));
      // compress undoes expand on the selected bits
      assert(std::bit_compress(std::bit_expand(x, m), m) == static_cast<T>(x & std::bit_compress(all, m)));
    }
  }
}

constexpr bool test() {
  test_type<unsigned char>();
  test_type<unsigned short>();
  test_type<unsigned int>();
  test_type<unsigned long>();
  test_type<unsigned long long>();
#ifndef TEST_HAS_NO_INT128
  test_type<__uint128_t>();
#endif

  // the examples of the draft
  assert(std::bit_repeat(std::uint32_t{0xc}, 4) == 0xcccccccc);
  // 0b ABCD, mask 0b0101: compress gives 0b00BD, expand gives 0b0C0D (A=1, B=0, C=1, D=1 here)
  assert(std::bit_compress(std::uint8_t{0b1011}, std::uint8_t{0b0101}) == 0b01);
  assert(std::bit_compress(std::uint8_t{0b0111}, std::uint8_t{0b0101}) == 0b11);
  assert(std::bit_expand(std::uint8_t{0b1011}, std::uint8_t{0b0101}) == 0b0101);
  assert(std::bit_expand(std::uint8_t{0b0010}, std::uint8_t{0b0101}) == 0b0100);
  assert(std::bit_reverse(std::uint8_t{0b10110000}) == 0b00001101);
  assert(std::bit_reverse(std::uint32_t{1}) == 0x80000000u);
  return true;
}

// Constraints: unsigned integer types only
template <class T>
concept has_bit_permutations = requires(T t) {
  std::bit_reverse(t);
  std::bit_repeat(t, 1);
  std::bit_compress(t, t);
  std::bit_expand(t, t);
};
static_assert(has_bit_permutations<unsigned char>);
static_assert(has_bit_permutations<unsigned long long>);
static_assert(!has_bit_permutations<int>);
static_assert(!has_bit_permutations<signed char>);
static_assert(!has_bit_permutations<char>);
static_assert(!has_bit_permutations<bool>);
static_assert(!has_bit_permutations<wchar_t>);
static_assert(!has_bit_permutations<char16_t>);
static_assert(!has_bit_permutations<float>);

// A call of bit_repeat that violates the precondition (l > 0) is not a core constant expression.
template <int L>
concept bit_repeat_is_constant = requires { typename std::integral_constant<unsigned, std::bit_repeat(1u, L)>; };
static_assert(bit_repeat_is_constant<1>);
static_assert(bit_repeat_is_constant<32>);
static_assert(!bit_repeat_is_constant<0>);
static_assert(!bit_repeat_is_constant<-3>);

int main(int, char**) {
  test();
  static_assert(test());
  return 0;
}
