//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20
// ADDITIONAL_COMPILE_FLAGS: -O2 -U__SIZEOF_INT128__

// Exhaustively compare narrow floating-point conversions with the exact narrow
// reference and check that every shortest result round-trips.

#include <__charconv/float128.h>
#include <cassert>
#include <charconv>
#include <cstdint>
#include <stdfloat>

template <class T>
void check_narrow(std::__f128::__fp_format format) {
  using namespace std;
  constexpr chars_format formats[] = {chars_format::scientific, chars_format::fixed, chars_format::hex, chars_format::general};
  constexpr std::__f128::__fmt_kind kinds[] = {
      std::__f128::__fmt_kind::__scientific,
      std::__f128::__fmt_kind::__fixed,
      std::__f128::__fmt_kind::__hex,
      std::__f128::__fmt_kind::__general};
  constexpr int precisions[] = {0, 1, 3, 6, 17};
  char output[128];
  char expected[128];

  for (unsigned bits = 0; bits != 65536; ++bits) {
    T value = __builtin_bit_cast(T, static_cast<uint16_t>(bits));
    auto shortest = to_chars(output, output + sizeof(output), value);
    assert(shortest.ec == errc{});
    auto expected_shortest = std::__f128::__to_chars_narrow(
        expected, expected + sizeof(expected), static_cast<unsigned short>(bits), std::__f128::__fmt_kind::__plain, format);
    assert(!expected_shortest.__too_large);
    assert(shortest.ptr - output == expected_shortest.__ptr - expected);
    for (ptrdiff_t i = 0; i != shortest.ptr - output; ++i)
      assert(output[i] == expected[i]);

    T parsed{};
    auto parsed_result = from_chars(output, shortest.ptr, parsed);
    assert(parsed_result.ec == errc{} && parsed_result.ptr == shortest.ptr);
    const unsigned exponent_mask = ((1u << format.__exp_bits) - 1) << format.__frac_bits;
    const unsigned fraction_mask = (1u << format.__frac_bits) - 1;
    const bool is_nan = (bits & exponent_mask) == exponent_mask && (bits & fraction_mask) != 0;
    assert(is_nan ? static_cast<float>(parsed) != static_cast<float>(parsed)
                  : __builtin_bit_cast(uint16_t, parsed) == bits);

    for (unsigned f = 0; f != 4; ++f) {
      auto converted = to_chars(output, output + sizeof(output), value, formats[f]);
      assert(converted.ec == errc{});
      if (f == 2) {
        auto expected_hex = to_chars(expected, expected + sizeof(expected), static_cast<float>(value), formats[f]);
        assert(expected_hex.ec == errc{} && converted.ptr - output == expected_hex.ptr - expected);
        for (ptrdiff_t i = 0; i != converted.ptr - output; ++i)
          assert(output[i] == expected[i]);
      } else {
        auto expected_formatted = std::__f128::__to_chars_narrow(
            expected, expected + sizeof(expected), static_cast<unsigned short>(bits), kinds[f], format);
        assert(!expected_formatted.__too_large && converted.ptr - output == expected_formatted.__ptr - expected);
        for (ptrdiff_t i = 0; i != converted.ptr - output; ++i)
          assert(output[i] == expected[i]);
      }

      for (int precision : precisions) {
        auto precise = to_chars(output, output + sizeof(output), value, formats[f], precision);
        auto expected_precise = to_chars(expected, expected + sizeof(expected), static_cast<float>(value), formats[f], precision);
        assert(precise.ec == errc{} && expected_precise.ec == errc{});
        assert(precise.ptr - output == expected_precise.ptr - expected);
        for (ptrdiff_t i = 0; i != precise.ptr - output; ++i)
          assert(output[i] == expected[i]);
      }
    }
  }
}

int main(int, char**) {
#if defined(__STDCPP_FLOAT16_T__)
  check_narrow<std::float16_t>(std::__f128::__binary16);
#endif
#if defined(__STDCPP_BFLOAT16_T__)
  check_narrow<std::bfloat16_t>(std::__f128::__bfloat16);
#endif
}
