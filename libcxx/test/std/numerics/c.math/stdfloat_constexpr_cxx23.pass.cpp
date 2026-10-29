//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

// <cmath>

// P0533R9: frexp, modf, remquo, nexttoward and fma are constexpr in C++23 for every floating-point-type overload,
// the extended floating-point types included (they were constexpr only since C++26, and fma(float128_t) never).

#include <cassert>
#include <cmath>
#include <stdfloat>

#include "test_macros.h"

template <class F>
constexpr bool check() {
  int e = 0;
  if (std::frexp(F(8), &e) != F(0.5) || e != 4)
    return false;
  F integral = 0;
  if (std::modf(F(2.5), &integral) != F(0.5) || integral != F(2))
    return false;
  int q = 0;
  if (std::remquo(F(5.5), F(2), &q) != F(-0.5) || q != 3)
    return false;
  if (std::nexttoward(F(1), 2.0L) <= F(1))
    return false;
  return true;
}

#ifdef __STDCPP_FLOAT16_T__
static_assert(check<std::float16_t>());
#endif
#ifdef __STDCPP_FLOAT32_T__
static_assert(check<std::float32_t>());
#endif
#ifdef __STDCPP_FLOAT64_T__
static_assert(check<std::float64_t>());
#endif
#ifdef __STDCPP_BFLOAT16_T__
static_assert(check<std::bfloat16_t>());
#endif
#if defined(__STDCPP_FLOAT128_T__) && defined(__GLIBC__)
static_assert(check<std::float128_t>());
static_assert(std::fma(2.0f128, 3.0f128, 1.0f128) == 7);
// (1 + e)^2 - (1 + 2e) = e^2: one rounding keeps it, a multiplication and an addition lose it.
constexpr std::float128_t eps = std::ldexp(1.0f128, -112);
static_assert(std::fma(1 + eps, 1 + eps, -(1 + 2 * eps)) == std::ldexp(1.0f128, -224));
#endif

int main(int, char**) { return 0; }
