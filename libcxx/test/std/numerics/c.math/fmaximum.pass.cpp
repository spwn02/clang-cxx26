//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <cmath>

// constexpr floating-point-type fmaximum(floating-point-type x, floating-point-type y);
// constexpr floating-point-type fmaximum_num(floating-point-type x, floating-point-type y);
// constexpr floating-point-type fminimum(floating-point-type x, floating-point-type y);
// constexpr floating-point-type fminimum_num(floating-point-type x, floating-point-type y);

#include <cassert>
#include <cmath>
#include <limits>
#include <math.h>
#include <type_traits>

#include "test_macros.h"

template <class T>
constexpr bool test() {
  const T nan = std::numeric_limits<T>::quiet_NaN();
  const T inf = std::numeric_limits<T>::infinity();

  // Ordinary values.
  assert(std::fmaximum(T(1), T(2)) == T(2));
  assert(std::fmaximum(T(2), T(1)) == T(2));
  assert(std::fminimum(T(1), T(2)) == T(1));
  assert(std::fminimum(T(2), T(1)) == T(1));
  assert(std::fmaximum_num(T(-1), T(-2)) == T(-1));
  assert(std::fminimum_num(T(-1), T(-2)) == T(-2));
  assert(std::fmaximum(-inf, T(1)) == T(1));
  assert(std::fminimum(inf, T(1)) == T(1));

  // -0 is less than +0.
  assert(!std::signbit(std::fmaximum(T(-0.0), T(0.0))));
  assert(!std::signbit(std::fmaximum(T(0.0), T(-0.0))));
  assert(std::signbit(std::fminimum(T(-0.0), T(0.0))));
  assert(std::signbit(std::fminimum(T(0.0), T(-0.0))));
  assert(!std::signbit(std::fmaximum_num(T(-0.0), T(0.0))));
  assert(std::signbit(std::fminimum_num(T(0.0), T(-0.0))));

  // The plain variants propagate a NaN, the _num variants prefer the number.
  assert(std::isnan(std::fmaximum(nan, T(1))));
  assert(std::isnan(std::fmaximum(T(1), nan)));
  assert(std::isnan(std::fminimum(nan, T(1))));
  assert(std::isnan(std::fminimum(T(1), nan)));
  assert(std::fmaximum_num(nan, T(1)) == T(1));
  assert(std::fmaximum_num(T(1), nan) == T(1));
  assert(std::fminimum_num(nan, T(1)) == T(1));
  assert(std::fminimum_num(T(1), nan) == T(1));
  assert(std::isnan(std::fmaximum_num(nan, nan)));
  assert(std::isnan(std::fminimum_num(nan, nan)));

  ASSERT_SAME_TYPE(decltype(std::fmaximum(T(), T())), T);
  ASSERT_SAME_TYPE(decltype(std::fminimum_num(T(), T())), T);
  return true;
}

constexpr bool test_all() {
  test<float>();
  test<double>();
  test<long double>();

  // Arithmetic promotion.
  ASSERT_SAME_TYPE(decltype(std::fmaximum(1, 2.0f)), double);
  ASSERT_SAME_TYPE(decltype(std::fminimum(1.0f, 2.0)), double);
  ASSERT_SAME_TYPE(decltype(std::fmaximum_num(1.0f, 2.0L)), long double);
  ASSERT_SAME_TYPE(decltype(std::fminimum_num(1, 2)), double);
  assert(std::fmaximum(1, 2.5f) == 2.5);
  assert(std::fminimum_num(3, 2) == 2.0);

  // The global names (from <math.h>) behave the same.
  assert(::fmaximum(1.0, 2.0) == 2.0);
  assert(::fminimum(1.0f, 2.0f) == 1.0f);
  assert(::fmaximum_num(1.0L, 2.0L) == 2.0L);
  assert(::fminimum_num(1.0, 2.0) == 1.0);
  return true;
}

int main(int, char**) {
  test_all();
  static_assert(test_all());
  return 0;
}
