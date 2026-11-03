//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14

// to_chars requires functions in the dylib that have not been introduced in older
// versions of the dylib on macOS.
// XFAIL: availability-fp_to_chars-missing

// <charconv>

// to_chars_result to_chars(char* first, char* last, floating-point-type value);
//
// P3505R4: the default representation uses the fixed notation if the absolute value is in [l, u) and the scientific
// notation otherwise, where l is the smallest value that is at least 1e-4 and u the largest power of ten that is at
// most radix^(digits + 1), both representable in the type. The significand is the shortest one that round-trips.

#include <charconv>
#include <cassert>
#include <limits>
#include <string>
#include <string_view>

#include "test_macros.h"

#if TEST_STD_VER >= 20
#  include <format>
#endif

template <class T>
std::string to_string(T value) {
  char buffer[128];
  std::to_chars_result r = std::to_chars(buffer, buffer + sizeof(buffer), value);
  assert(r.ec == std::errc{});
  return std::string(buffer, r.ptr);
}

bool starts(const std::string& s, std::string_view prefix) { return s.compare(0, prefix.size(), prefix) == 0; }

template <class T>
void test_common() {
  // The motivating examples: the size of the output does not matter.
  assert(to_string(T(100000)) == "100000");
  assert(to_string(T(120000)) == "120000");
  assert(to_string(T(1000)) == "1000");
  assert(to_string(T(-100000)) == "-100000");

  // Zero is printed as "0" in either sign.
  assert(to_string(T(0)) == "0");
  assert(to_string(-T(0)) == "-0");

  // Ordinary values keep their shortest significand.
  assert(to_string(T(0.5)) == "0.5");
  assert(to_string(T(1)) == "1");
  assert(to_string(T(123.25)) == "123.25");
  assert(to_string(T(-0.015625)) == "-0.015625");

  // Everything below the lower bound and everything from the upper bound on is scientific.
  // (the digits of the shortest significand depend on the type, the notation does not)
  assert(starts(to_string(T(1) / T(65536)), "1.5258") && to_string(T(1) / T(65536)).find("e-05") != std::string::npos);
  assert(starts(to_string(T(1) / T(8192)), "0.000122")); // 2^-13 = 1.220703125e-4, above the lower bound
  assert(starts(to_string(T(1) / T(16384)), "6.1035") && to_string(T(1) / T(16384)).find("e-05") != std::string::npos);
  assert(to_string(std::numeric_limits<T>::min()).find('e') != std::string::npos);
  assert(to_string(std::numeric_limits<T>::max()).find('e') != std::string::npos);
  assert(to_string(std::numeric_limits<T>::denorm_min()).find('e') != std::string::npos);

  // The decimal exponent of the shortest significand does not decide: 2^-12 = 0.000244140625 has ten digits.
  assert(starts(to_string(T(1) / T(4096)), "0.000244140"));
}

void test_float() {
  test_common<float>();
  // The upper bound is 1e7 (radix^25 = 33554432 rounds down to the power of ten 1e7).
  assert(to_string(9999999.0f) == "9999999");
  assert(to_string(1e7f) == "1e+07");
  assert(to_string(1.5e7f) == "1.5e+07");
  assert(to_string(12345678.0f) == "1.2345678e+07");
  // The lower bound is the smallest float that is at least 1e-4. The float closest to 1e-4 is smaller than 1e-4.
  assert(to_string(0.00010000001f) == "0.00010000001");
  assert(to_string(1e-4f) == "1e-04");
}

void test_double() {
  test_common<double>();
  // The upper bound is 1e16 (radix^54 = 18014398509481984).
  assert(to_string(1e15) == "1000000000000000");
  assert(to_string(9999999999999998.0) == "9999999999999998");
  assert(to_string(1e16) == "1e+16");
  assert(to_string(1.5e16) == "1.5e+16");
  // The double closest to 1e-4 is larger than 1e-4, so 1e-4 is the lower bound itself.
  assert(to_string(1e-4) == "0.0001");
  assert(to_string(9.999999999999999e-5) == "9.999999999999999e-05");
  // No digits beyond the shortest significand ("garbage digits").
  assert(to_string(1234567890123456700000.0) == "1.2345678901234568e+21");
  assert(to_string(0.1) == "0.1");
}

void test_long_double() {
  test_common<long double>();
  if constexpr (std::numeric_limits<long double>::digits == 64) {
    // x87: radix^65 = 36893488147419103232, the upper bound is 1e19.
    assert(to_string(1e18L) == "1000000000000000000");
    assert(to_string(1e19L) == "1e+19");
    assert(to_string(1.5e19L) == "1.5e+19");
    assert(to_string(1e-4L) == "0.0001");
  } else if constexpr (std::numeric_limits<long double>::digits == 53) {
    assert(to_string(1e16L) == "1e+16");
    assert(to_string(1e15L) == "1000000000000000");
  }
}

#if TEST_STD_VER >= 20
void test_format() {
  // std::format with the default format uses the same representation.
  assert(std::format("{}", 100000.0) == "100000");
  assert(std::format("{}", 120000.0) == "120000");
  assert(std::format("{}", 1234567890123456700000.0) == "1.2345678901234568e+21");
  assert(std::format("{}", 1e16) == "1e+16");
  assert(std::format("{}", 0.0) == "0");
  assert(std::format("{}", -0.0) == "-0");
  assert(std::format("{}", 1e7f) == "1e+07");
  assert(std::format("{:>10}", 100000.0) == "    100000");
  // Explicit formats are unchanged.
  assert(std::format("{:e}", 100000.0) == "1.000000e+05");
  assert(std::format("{:g}", 100000.0) == "100000");
  assert(std::format("{:g}", 1000000.0) == "1e+06");
}
#endif

int main(int, char**) {
  test_float();
  test_double();
  test_long_double();
#if TEST_STD_VER >= 20
  test_format();
#endif
  return 0;
}
