//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <cmath>

// The overloads of <cmath> for std::float128_t are constexpr ([cmath.syn]): the exact functions (P0533R9) and the
// transcendental ones (P1383R2). The compiler evaluates them with the builtins; at run time the same entry points of
// the C library are called, so the two agree up to the accuracy of that library.

#include <cassert>
#include <cmath>
#include <limits>
#include <stdfloat>

#include "test_macros.h"

#if defined(__STDCPP_FLOAT128_T__) && defined(__GLIBC__)

using F = std::float128_t;

// The exact functions.
static_assert(std::ceil(1.5f128) == 2);
static_assert(std::floor(-1.5f128) == -2);
static_assert(std::round(2.5f128) == 3);
static_assert(std::trunc(-2.75f128) == -2);
static_assert(std::logb(8.0f128) == 3);
static_assert(std::fmod(5.5f128, 2.0f128) == 1.5f128);
static_assert(std::remainder(5.0f128, 3.0f128) == -1);
static_assert(std::copysign(1.5f128, -1.0f128) == -1.5f128);
static_assert(std::fdim(5.0f128, 3.0f128) == 2);
static_assert(std::fmax(1.0f128, 2.0f128) == 2);
static_assert(std::fmin(1.0f128, 2.0f128) == 1);
static_assert(std::nextafter(1.0f128, 2.0f128) == 1 + std::numeric_limits<F>::epsilon());
static_assert(std::nexttoward(1.0f128, 2.0L) == 1 + std::numeric_limits<F>::epsilon());
static_assert(std::ilogb(8.0f128) == 3);
static_assert(std::lround(2.5f128) == 3);
static_assert(std::llround(-2.5f128) == -3);
static_assert(std::ldexp(1.5f128, 3) == 12);
static_assert(std::scalbn(1.5f128, 3) == 12);
static_assert(std::scalbln(1.5f128, 3L) == 12);
constexpr bool pointer_functions() {
  int e = 0;
  if (std::frexp(8.0f128, &e) != 0.5f128 || e != 4)
    return false;
  F integral = 0;
  if (std::modf(2.5f128, &integral) != 0.5f128 || integral != 2)
    return false;
  int q = 0;
  if (std::remquo(5.5f128, 2.0f128, &q) != -0.5f128 || q != 3)
    return false;
  return true;
}
static_assert(pointer_functions());

// The transcendental functions, at arguments with an exact result.
static_assert(std::sin(0.0f128) == 0);
static_assert(std::cos(0.0f128) == 1);
static_assert(std::tan(0.0f128) == 0);
static_assert(std::asin(0.0f128) == 0);
static_assert(std::acos(1.0f128) == 0);
static_assert(std::atan(0.0f128) == 0);
static_assert(std::atan2(0.0f128, 1.0f128) == 0);
static_assert(std::sinh(0.0f128) == 0);
static_assert(std::cosh(0.0f128) == 1);
static_assert(std::tanh(0.0f128) == 0);
static_assert(std::asinh(0.0f128) == 0);
static_assert(std::acosh(1.0f128) == 0);
static_assert(std::atanh(0.0f128) == 0);
static_assert(std::exp(0.0f128) == 1);
static_assert(std::exp2(10.0f128) == 1024);
static_assert(std::expm1(0.0f128) == 0);
static_assert(std::log(1.0f128) == 0);
static_assert(std::log2(8.0f128) == 3);
static_assert(std::log10(1000.0f128) == 3);
static_assert(std::log1p(0.0f128) == 0);
static_assert(std::pow(2.0f128, 10.0f128) == 1024);
static_assert(std::hypot(3.0f128, 4.0f128) == 5);
static_assert(std::cbrt(27.0f128) == 3);
static_assert(std::erf(0.0f128) == 0);
static_assert(std::erfc(0.0f128) == 1);
static_assert(std::tgamma(5.0f128) == 24);
static_assert(std::lgamma(1.0f128) == 0);

// A value that is not exact: the constant evaluation and the C library agree to the precision of the format.
constexpr F sin_constant = std::sin(0.7f128);
constexpr F exp_constant = std::exp(1.5f128);
constexpr F log_constant = std::log(7.0f128);
constexpr F pow_constant = std::pow(1.7f128, 2.3f128);

static bool close(F a, F b) {
  const F diff = a > b ? a - b : b - a;
  return diff <= std::numeric_limits<F>::epsilon() * 64 * (a > 0 ? a : -a);
}

int main(int, char**) {
  volatile F x = 0.7f128, y = 1.5f128, z = 7.0f128, p = 1.7f128, q = 2.3f128;
  assert(close(sin_constant, std::sin(F(x))));
  assert(close(exp_constant, std::exp(F(y))));
  assert(close(log_constant, std::log(F(z))));
  assert(close(pow_constant, std::pow(F(p), F(q))));
  return 0;
}

#else

int main(int, char**) { return 0; }

#endif
