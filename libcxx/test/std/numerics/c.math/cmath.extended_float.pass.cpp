//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// The [cpp.predefined] __STDCPP_FLOAT<N>_T__ macros are defined by the frontend only for libc++ users.
// ADDITIONAL_COMPILE_FLAGS: -Xclang -fstdcpp-float-macros

// <cmath>

// fmaximum, fminimum, fmaximum_num, fminimum_num, hypot (three arguments) and lerp accept every extended
// floating-point type, and std::float128_t also has ilogb, lrint, llrint, lround, llround, scalbln and nexttoward.

#include <cassert>
#include <cmath>
#include <limits>
#include <stdfloat>

template <class F>
void test_common() {
  assert(std::fmaximum(F(1), F(2)) == F(2));
  assert(std::fminimum(F(1), F(2)) == F(1));
  assert(std::isnan(std::fmaximum(F(1), std::numeric_limits<F>::quiet_NaN())));
  assert(std::isnan(std::fminimum(F(1), std::numeric_limits<F>::quiet_NaN())));
  assert(std::fmaximum_num(F(1), std::numeric_limits<F>::quiet_NaN()) == F(1));
  assert(std::fminimum_num(F(3), std::numeric_limits<F>::quiet_NaN()) == F(3));
  assert(!std::signbit(std::fmaximum(F(-0.0), F(0.0))));
  assert(std::signbit(std::fminimum(F(-0.0), F(0.0))));
  assert(std::hypot(F(3), F(4), F(12)) == F(13));
  assert(std::lerp(F(0), F(10), F(0.5)) == F(5));
}

int main(int, char**) {
#if defined(__STDCPP_FLOAT16_T__)
  test_common<std::float16_t>();
#endif
#if defined(__STDCPP_BFLOAT16_T__)
  test_common<std::bfloat16_t>();
#endif
#if defined(__STDCPP_FLOAT32_T__)
  test_common<std::float32_t>();
#endif
#if defined(__STDCPP_FLOAT64_T__)
  test_common<std::float64_t>();
#endif
#if defined(__STDCPP_FLOAT128_T__)
  test_common<std::float128_t>();
  assert(std::ilogb(std::float128_t(8)) == 3);
  assert(std::lrint(std::float128_t(2.5)) == 2);
  assert(std::llrint(std::float128_t(3.5)) == 4);
  assert(std::lround(std::float128_t(2.5)) == 3);
  assert(std::llround(std::float128_t(-2.5)) == -3);
  assert(std::scalbln(std::float128_t(1), 10L) == std::float128_t(1024));
  assert(std::nexttoward(std::float128_t(1), 2.0L) > std::float128_t(1));
#endif
  return 0;
}
