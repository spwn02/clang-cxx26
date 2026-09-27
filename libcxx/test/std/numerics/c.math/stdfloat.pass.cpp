//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

#include <cassert>
#include <cmath>
#include <stdfloat>
#include <type_traits>

#include "test_macros.h"

int main(int, char**) {
  using std::bfloat16_t;
  using std::float16_t;
  using std::float32_t;
  using std::float64_t;
  assert(std::sqrt(float32_t(4)) == float32_t(std::sqrt(4.0f)));
  assert(std::sqrt(float64_t(4)) == float64_t(std::sqrt(4.0)));
  assert(std::pow(float32_t(2), 3.0f) == float32_t(8));
  assert(std::fmod(float32_t(7), 2.0f) == float32_t(1));
  assert(std::exp(float32_t(0)) == float32_t(1));
  assert(std::log(float64_t(1)) == float64_t(0));
  assert(std::sin(float32_t(0)) == float32_t(0));
  assert(std::cos(float64_t(0)) == float64_t(1));
  assert(std::tan(float32_t(0)) == float32_t(0));
  assert(std::atan2(float32_t(0), float32_t(1)) == float32_t(0));
  assert(std::hypot(float16_t(3), float16_t(4)) == float16_t(5));
  assert(std::fma(bfloat16_t(2), bfloat16_t(3), bfloat16_t(1)) == bfloat16_t(7));
  assert(std::fabs(float16_t(-2)) == float16_t(2));
  assert(std::floor(bfloat16_t(2.5)) == bfloat16_t(2));
  assert(std::ceil(float16_t(2.5)) == float16_t(3));
  assert(std::round(float32_t(2.5)) == float32_t(3));
  assert(std::trunc(float64_t(2.5)) == float64_t(2));
  assert(std::isfinite(float16_t(1)) && !std::isnan(float32_t(1)) && !std::isinf(float64_t(1)));
  assert(std::signbit(float16_t(-0.0)));
  assert(std::fpclassify(float32_t(0)) == FP_ZERO);
  assert(std::copysign(float32_t(2), float32_t(-1)) == float32_t(-2));
  assert(std::ldexp(float16_t(1), 2) == float16_t(4));
  int exponent = 0;
  assert(std::frexp(float64_t(8), &exponent) == float64_t(0.5) && exponent == 4);
  assert(std::scalbn(float32_t(1), 2) == float32_t(4));
  assert(std::lerp(float32_t(2), float32_t(4), float32_t(0.5)) == float32_t(3));
  return 0;
}
