//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

// <cmath>

// [cmath.syn]: the C-named float and long double functions (fabsf, fmal, nextafterl, ...) are constexpr since
// C++23 (P0533R9), nextup/nextdown since C++26, both as std:: names and as global names.

#include <cmath>
#include <math.h>

#include "test_macros.h"

static_assert(std::fabsf(-2.5f) == 2.5f);
static_assert(std::fabsl(-2.5L) == 2.5L);
static_assert(std::fmaf(2.0f, 3.0f, 1.0f) == 7.0f);
static_assert(std::fmal(2.0L, 3.0L, 1.0L) == 7.0L);
static_assert(std::floorf(1.5f) == 1.0f);
static_assert(std::ceill(1.5L) == 2.0L);
static_assert(std::nextafterf(1.0f, 2.0f) == 1.0000001f);
static_assert(std::nextafterl(1.0L, 0.0L) < 1.0L);
static_assert(::fabsf(-2.5f) == 2.5f);
static_assert(::fmal(2.0L, 3.0L, 1.0L) == 7.0L);
static_assert(::nextafterf(1.0f, 2.0f) == 1.0000001f);

constexpr bool modf_ok() {
  float f   = 0;
  long double l = 0;
  return std::modff(2.5f, &f) == 0.5f && f == 2.0f && std::modfl(2.5L, &l) == 0.5L && l == 2.0L;
}
static_assert(modf_ok());

#if TEST_STD_VER >= 26
static_assert(std::nextup(1.0) == 1.0000000000000002);
static_assert(std::nextup(1.0f) == 1.0000001f);
static_assert(std::nextup(1.0L) > 1.0L);
static_assert(std::nextup(1) > 1.0);
static_assert(std::nextdown(1.0) == 0.9999999999999999);
static_assert(std::nextdown(1.0f) < 1.0f);
static_assert(std::nextdown(1.0L) < 1.0L);
static_assert(std::nextdown(1) < 1.0);
static_assert(std::nextup(0.0) == 4.9406564584124654e-324);
static_assert(std::nextdown(0.0) == -4.9406564584124654e-324);
static_assert(std::nextup(-0.0f) > 0.0f);
static_assert(std::nextup(INFINITY) == INFINITY);
static_assert(std::nextdown(-INFINITY) == -INFINITY);
static_assert(std::nextdown(INFINITY) < INFINITY);
static_assert(std::isnan(std::nextup(NAN)));
static_assert(std::isnan(std::nextdown(NAN)));
static_assert(std::nextupf(1.0f) == 1.0000001f);
static_assert(std::nextdownf(1.0f) < 1.0f);
static_assert(std::nextupl(1.0L) > 1.0L);
static_assert(std::nextdownl(1.0L) < 1.0L);
static_assert(::nextup(1.0) == 1.0000000000000002);
static_assert(::nextdownf(1.0f) < 1.0f);
#endif // TEST_STD_VER >= 26

int main(int, char**) { return 0; }
