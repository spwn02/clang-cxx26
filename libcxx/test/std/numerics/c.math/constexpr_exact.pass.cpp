//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

// <cmath>

// P0533R9: the functions of <cmath> whose results are exactly computable are constexpr in C++23.
// (div/ldiv/lldiv, remquo and the C-named float/long double variants are not covered yet.)

#include <cmath>
#include <type_traits>

static_assert(std::fdim(3.0, 1.0) == 2.0);
static_assert(std::fdim(3.0f, 1.0f) == 2.0f);
static_assert(std::fdim(3.0L, 1.0L) == 2.0L);
static_assert(std::fdim(3, 1.5) == 1.5);
static_assert(std::fdim(1.0, 3.0) == 0.0);

static_assert(std::fma(2.0, 3.0, 1.0) == 7.0);
static_assert(std::fma(2.0f, 3.0f, 1.0f) == 7.0f);
static_assert(std::fma(2, 3.0, 1.0f) == 7.0);
static_assert(std::fma(0.1, 10.0, -1.0) != 0.0); // a single rounding

static_assert(std::ldexp(1.5, 4) == 24.0);
static_assert(std::ldexp(1.5f, 4) == 24.0f);
static_assert(std::ldexp(3, 4) == 48.0);
static_assert(std::scalbn(1.0, 10) == 1024.0);
static_assert(std::scalbn(1.0L, 10) == 1024.0L);

static_assert(std::ilogb(8.0) == 3);
static_assert(std::ilogb(8.0f) == 3);
static_assert(std::ilogb(8) == 3);

static_assert(std::nextafter(1.0, 2.0) > 1.0);
static_assert(std::nextafter(1.0, 0.0) < 1.0);
static_assert(std::nextafter(1.0f, 2.0f) > 1.0f);
static_assert(std::nextafter(1.0L, 2.0L) > 1.0L);
static_assert(std::nexttoward(1.0, 2.0L) > 1.0);
static_assert(std::nexttoward(1.0f, 2.0L) > 1.0f);

static_assert(std::isgreater(2.0, 1.0));
static_assert(std::isgreaterequal(2.0, 2.0));
static_assert(std::isless(1, 2.0f));
static_assert(std::islessequal(1.0, 2.0));
static_assert(std::islessgreater(1.0, 2.0));
static_assert(std::isunordered(__builtin_nan(""), 1.0));

constexpr bool test_out_parameters() {
  int e = 0;
  double m = std::frexp(48.0, &e);
  if (m != 0.75 || e != 6)
    return false;
  float mf = std::frexp(48.0f, &e);
  long double ml = std::frexp(48.0L, &e);
  double mi = std::frexp(48, &e);
  if (mf != 0.75f || ml != 0.75L || mi != 0.75)
    return false;

  double i = 0;
  double f = std::modf(-3.25, &i);
  if (f != -0.25 || i != -3.0)
    return false;
  float fi = 0;
  float ff = std::modf(2.5f, &fi);
  long double li = 0;
  long double lf = std::modf(0.75L, &li);
  return ff == 0.5f && fi == 2.0f && lf == 0.75L && li == 0.0L;
}
static_assert(test_out_parameters());

int main(int, char**) { return 0; }
