// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// REQUIRES: has-constexpr-mpfr-math
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <cmath>
#include <limits>

static_assert(std::sqrt(4.0) == 2.0);
static_assert(std::pow(2.0, 10.0) == 1024.0);
static_assert(std::exp(0.0) == 1.0);
static_assert(std::log(1.0) == 0.0);
static_assert(std::sin(-0.0) == 0.0 && std::signbit(std::sin(-0.0)));
static_assert(std::cos(0.0) == 1.0);
static_assert(std::atan2(-0.0, 1.0) == 0.0 && std::signbit(std::atan2(-0.0, 1.0)));
static_assert(std::hypot(3.0, 4.0) == 5.0);
static_assert(std::hypot(2.0, 3.0, 6.0) == 7.0);
static_assert(std::hypot(2.0f, 3.0f, 6.0f) == 7.0f);
static_assert(std::hypot(2.0L, 3.0L, 6.0L) == 7.0L);
static_assert(std::hypot(1e300, 1e300, 1e300) > 1.7e300);
static_assert(std::sqrt(4.0f) == 2.0f);
static_assert(std::sqrt(4.0L) == 2.0L);
static_assert(std::sin(0.0) == 0.0);
static_assert(std::sin(0.0f) == 0.0f);
static_assert(std::sin(0.0L) == 0.0L);
static_assert(std::sin(0) == 0.0);
static_assert(std::erf(0.0) == 0.0);
static_assert(std::erfc(0.0) == 1.0);
static_assert(std::tgamma(1.0) == 1.0);
static_assert(std::lgamma(1.0) == 0.0);
static_assert(std::sin(1.0) == 0x1.aed548f090ceep-1);
static_assert(std::pow(2.0, -1074.0) == 0x0.0000000000001p-1022);

int main() {
  volatile double input = 4.0;
  return std::sqrt(input) == 2.0 ? 0 : 1;
}
