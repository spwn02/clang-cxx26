//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <stdbit.h>

// P3370R1: bit and byte utilities, adopted from C23.
//
//   template<class T> unsigned int stdc_leading_zeros(T value);
//   unsigned int stdc_leading_zeros_uc(unsigned char value);
//   unsigned int stdc_leading_zeros_us(unsigned short value);
//   unsigned int stdc_leading_zeros_ui(unsigned int value);
//   unsigned int stdc_leading_zeros_ul(unsigned long value);
//   unsigned int stdc_leading_zeros_ull(unsigned long long value);
//   (same shape for leading_ones, trailing_zeros, trailing_ones,
//   first_leading_zero, first_leading_one, first_trailing_zero,
//   first_trailing_one, count_zeros, count_ones)
//
//   template<class T> bool stdc_has_single_bit(T value);
//   (plus _uc/_us/_ui/_ul/_ull overloads)
//
//   template<class T> unsigned int stdc_bit_width(T value);
//   (plus _uc/_us/_ui/_ul/_ull overloads)
//
//   template<class T> T stdc_bit_floor(T value);
//   template<class T> T stdc_bit_ceil(T value);
//   (plus _uc/_us/_ui/_ul/_ull overloads)

#include <stdbit.h>

#include <cassert>
#include <cstdint>
#include <limits>

#include "test_macros.h"

#ifndef __STDC_VERSION_STDBIT_H__
#  error __STDC_VERSION_STDBIT_H__ should be defined
#endif
static_assert(__STDC_VERSION_STDBIT_H__ == 202311L, "");

#ifndef __STDC_ENDIAN_LITTLE__
#  error __STDC_ENDIAN_LITTLE__ should be defined
#endif
#ifndef __STDC_ENDIAN_BIG__
#  error __STDC_ENDIAN_BIG__ should be defined
#endif
#ifndef __STDC_ENDIAN_NATIVE__
#  error __STDC_ENDIAN_NATIVE__ should be defined
#endif
static_assert(__STDC_ENDIAN_NATIVE__ == __STDC_ENDIAN_LITTLE__ || __STDC_ENDIAN_NATIVE__ == __STDC_ENDIAN_BIG__, "");

template <class T>
void test_leading_trailing() {
  constexpr unsigned digits = std::numeric_limits<T>::digits;

  assert(stdc_leading_zeros(T(0)) == digits);
  assert(stdc_leading_zeros(T(1)) == digits - 1);
  assert(stdc_leading_zeros(std::numeric_limits<T>::max()) == 0);

  assert(stdc_leading_ones(T(0)) == 0);
  assert(stdc_leading_ones(std::numeric_limits<T>::max()) == digits);
  assert(stdc_leading_ones(T(~T(1))) == digits - 1);

  assert(stdc_trailing_zeros(T(0)) == digits);
  assert(stdc_trailing_zeros(T(1)) == 0);
  assert(stdc_trailing_zeros(T(2)) == 1);

  assert(stdc_trailing_ones(T(0)) == 0);
  assert(stdc_trailing_ones(std::numeric_limits<T>::max()) == digits);
  assert(stdc_trailing_ones(T(1)) == 1);

  // First (i.e. most-significant) zero/one bit, one-based index from the left.
  assert(stdc_first_leading_zero(std::numeric_limits<T>::max()) == 0);
  assert(stdc_first_leading_zero(T(0)) == 1);
  assert(stdc_first_leading_zero(T(~(T(1) << (digits - 2)))) == 2);

  assert(stdc_first_leading_one(T(0)) == 0);
  assert(stdc_first_leading_one(std::numeric_limits<T>::max()) == 1);
  assert(stdc_first_leading_one(T(T(1) << (digits - 2))) == 2);

  // First (i.e. least-significant) zero/one bit, one-based index from the right.
  assert(stdc_first_trailing_zero(std::numeric_limits<T>::max()) == 0);
  assert(stdc_first_trailing_zero(T(0)) == 1);
  assert(stdc_first_trailing_zero(T(1)) == 2);

  assert(stdc_first_trailing_one(T(0)) == 0);
  assert(stdc_first_trailing_one(T(1)) == 1);
  assert(stdc_first_trailing_one(T(2)) == 2);

  assert(stdc_count_zeros(T(0)) == digits);
  assert(stdc_count_zeros(std::numeric_limits<T>::max()) == 0);

  assert(stdc_count_ones(T(0)) == 0);
  assert(stdc_count_ones(std::numeric_limits<T>::max()) == digits);
  assert(stdc_count_ones(T(3)) == 2);

  assert(stdc_has_single_bit(T(0)) == false);
  assert(stdc_has_single_bit(T(1)) == true);
  assert(stdc_has_single_bit(T(2)) == true);
  assert(stdc_has_single_bit(T(3)) == false);

  assert(stdc_bit_width(T(0)) == 0);
  assert(stdc_bit_width(T(1)) == 1);
  assert(stdc_bit_width(T(3)) == 2);

  assert(stdc_bit_floor(T(0)) == T(0));
  assert(stdc_bit_floor(T(5)) == T(4));
  assert(stdc_bit_floor(std::numeric_limits<T>::max()) == T(T(1) << (digits - 1)));

  // Unlike std::bit_ceil, stdc_bit_ceil returns 0 (not UB) when the result
  // isn't representable in T.
  assert(stdc_bit_ceil(T(0)) == T(1));
  assert(stdc_bit_ceil(T(5)) == T(8));
  assert(stdc_bit_ceil(std::numeric_limits<T>::max()) == T(0));
  // Boundary case: the largest power of two that fits in T is itself
  // representable (it isn't "greater than the largest representable power
  // of two", it *is* the largest one) — must round-trip to itself, not 0.
  {
    constexpr T __max_pow2 = T(T(1) << (digits - 1));
    assert(stdc_bit_ceil(__max_pow2) == __max_pow2);
    assert(stdc_bit_ceil(T(__max_pow2 + 1)) == T(0));
  }
}

void test_fixed_width_overloads() {
  assert(stdc_leading_zeros_uc((unsigned char)0x01) == 7);
  assert(stdc_leading_zeros_us((unsigned short)0x0001) == 15);
  assert(stdc_leading_zeros_ui(0x00000001u) == 31);
  assert(stdc_leading_zeros_ul(0x00000001ul) >= 31); // width of `unsigned long` is platform-dependent
  assert(stdc_leading_zeros_ull(0x0000000000000001ull) == 63);

  assert(stdc_leading_ones_uc((unsigned char)0xFE) == 7);
  assert(stdc_leading_ones_us((unsigned short)0xFFFE) == 15);
  assert(stdc_leading_ones_ui(0xFFFFFFFEu) == 31);
  assert(stdc_leading_ones_ull(0xFFFFFFFFFFFFFFFEull) == 63);

  assert(stdc_trailing_zeros_uc((unsigned char)0x80) == 7);
  assert(stdc_trailing_zeros_us((unsigned short)0x8000) == 15);
  assert(stdc_trailing_zeros_ui(0x80000000u) == 31);
  assert(stdc_trailing_zeros_ull(0x8000000000000000ull) == 63);

  assert(stdc_trailing_ones_uc((unsigned char)0x7F) == 7);
  assert(stdc_trailing_ones_us((unsigned short)0x7FFF) == 15);
  assert(stdc_trailing_ones_ui(0x7FFFFFFFu) == 31);
  assert(stdc_trailing_ones_ull(0x7FFFFFFFFFFFFFFFull) == 63);

  assert(stdc_first_leading_zero_uc((unsigned char)0xFF) == 0);
  assert(stdc_first_leading_zero_us((unsigned short)0xFFFF) == 0);
  assert(stdc_first_leading_zero_ui(0xFFFFFFFFu) == 0);
  assert(stdc_first_leading_zero_ull(0xFFFFFFFFFFFFFFFFull) == 0);

  assert(stdc_first_leading_one_uc((unsigned char)0x00) == 0);
  assert(stdc_first_leading_one_us((unsigned short)0x0000) == 0);
  assert(stdc_first_leading_one_ui(0x00000000u) == 0);
  assert(stdc_first_leading_one_ull(0x0000000000000000ull) == 0);

  assert(stdc_first_trailing_zero_uc((unsigned char)0xFF) == 0);
  assert(stdc_first_trailing_zero_us((unsigned short)0xFFFF) == 0);
  assert(stdc_first_trailing_zero_ui(0xFFFFFFFFu) == 0);
  assert(stdc_first_trailing_zero_ull(0xFFFFFFFFFFFFFFFFull) == 0);

  assert(stdc_first_trailing_one_uc((unsigned char)0x00) == 0);
  assert(stdc_first_trailing_one_us((unsigned short)0x0000) == 0);
  assert(stdc_first_trailing_one_ui(0x00000000u) == 0);
  assert(stdc_first_trailing_one_ull(0x0000000000000000ull) == 0);

  assert(stdc_count_zeros_uc((unsigned char)0x00) == 8);
  assert(stdc_count_zeros_us((unsigned short)0x0000) == 16);
  assert(stdc_count_zeros_ui(0x00000000u) == 32);
  assert(stdc_count_zeros_ull(0x0000000000000000ull) == 64);

  assert(stdc_count_ones_uc((unsigned char)0xFF) == 8);
  assert(stdc_count_ones_us((unsigned short)0xFFFF) == 16);
  assert(stdc_count_ones_ui(0xFFFFFFFFu) == 32);
  assert(stdc_count_ones_ull(0xFFFFFFFFFFFFFFFFull) == 64);

  assert(stdc_has_single_bit_uc((unsigned char)0x10) == true);
  assert(stdc_has_single_bit_us((unsigned short)0x10) == true);
  assert(stdc_has_single_bit_ui(0x10u) == true);
  assert(stdc_has_single_bit_ull(0x10ull) == true);
  assert(stdc_has_single_bit_uc((unsigned char)0x11) == false);

  assert(stdc_bit_width_uc((unsigned char)0x11) == 5);
  assert(stdc_bit_width_us((unsigned short)0x11) == 5);
  assert(stdc_bit_width_ui(0x11u) == 5);
  assert(stdc_bit_width_ull(0x11ull) == 5);

  assert(stdc_bit_floor_uc((unsigned char)17) == 16);
  assert(stdc_bit_floor_us((unsigned short)17) == 16);
  assert(stdc_bit_floor_ui(17u) == 16);
  assert(stdc_bit_floor_ull(17ull) == 16);

  assert(stdc_bit_ceil_uc((unsigned char)17) == 32);
  assert(stdc_bit_ceil_us((unsigned short)17) == 32);
  assert(stdc_bit_ceil_ui(17u) == 32);
  assert(stdc_bit_ceil_ull(17ull) == 32);
  // Not representable in unsigned char: stdc_bit_ceil returns 0, not UB.
  assert(stdc_bit_ceil_uc((unsigned char)0xFF) == 0);
  // Boundary: 0x80 is itself the largest power of two representable in
  // unsigned char, so it must round-trip, not collapse to 0.
  assert(stdc_bit_ceil_uc((unsigned char)0x80) == 0x80);
  assert(stdc_bit_ceil_uc((unsigned char)0x81) == 0);
}

int main(int, char**) {
  test_leading_trailing<unsigned char>();
  test_leading_trailing<unsigned short>();
  test_leading_trailing<unsigned int>();
  test_leading_trailing<unsigned long>();
  test_leading_trailing<unsigned long long>();
#ifndef TEST_HAS_NO_INT128
  test_leading_trailing<__uint128_t>();
#endif

  test_fixed_width_overloads();

  return 0;
}
