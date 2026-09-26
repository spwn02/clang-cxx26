//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17

// <format>

// std::formatter<long double> formats at the precision of long double instead of going through double, so the
// output is what std::to_chars produces ([format.string.std]).

#include <cassert>
#include <cfloat>
#include <charconv>
#include <format>
#include <string>

#include "test_macros.h"

static std::string to_chars_string(long double value, std::chars_format fmt) {
  char buffer[128];
  auto r = std::to_chars(buffer, buffer + sizeof(buffer), value, fmt);
  assert(r.ec == std::errc{});
  return std::string(buffer, r.ptr);
}

int main(int, char**) {
  // The shortest round-trip form and the hexfloat form must match std::to_chars for values that are not
  // exactly representable as a double.
  const long double values[] = {0.1L, 1.0L / 3, 4e30L, 1e-4000L, 1e4000L, LDBL_MAX, LDBL_MIN, LDBL_TRUE_MIN};
  for (long double v : values) {
    assert(std::format("{}", v) == to_chars_string(v, std::chars_format{}));
    assert(std::format("{:a}", v) == to_chars_string(v, std::chars_format::hex));
    assert(std::format("{}", -v) == "-" + to_chars_string(v, std::chars_format{}));
  }

  assert(std::format("{}", 0.1L) == "0.1");
  assert(std::format("{:#}", 4e30L) == "4.e+30");
  if constexpr (LDBL_MANT_DIG == 64)
    assert(std::format("{:.25f}", 0.1L) == "0.1000000000000000000013553");

  if constexpr (LDBL_MANT_DIG > DBL_MANT_DIG) {
    // A long double carries digits a double cannot: 1 + 2^-52 is a double, 1 + 2^-63 is not.
    long double x = 1.0L + 0x1p-63L;
    assert(std::format("{}", x) != "1");
    assert(std::format("{:a}", x) == "1.0000000000000002p+0");
  }

  // Precisions beyond what a stack buffer holds.
  std::string wide = std::format("{:.5000f}", 1e-4000L);
  assert(wide.size() == 5002);
  assert(std::format("{:.0f}", LDBL_MAX).size() == static_cast<std::size_t>(LDBL_MAX_10_EXP) + 1);

  // The wide-character formatter goes through the same code.
#ifndef TEST_HAS_NO_WIDE_CHARACTERS
  assert(std::format(L"{}", 0.1L) == L"0.1");
  assert(std::format(L"{:a}", 1.5L) == L"1.8p+0");
#endif

  return 0;
}
