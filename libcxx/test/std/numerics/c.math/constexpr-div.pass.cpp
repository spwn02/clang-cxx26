//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

// <cstdlib>

// constexpr div_t div(int, int);
// constexpr ldiv_t div(long, long);
// constexpr lldiv_t div(long long, long long);
// constexpr ldiv_t ldiv(long, long);
// constexpr lldiv_t lldiv(long long, long long);
// constexpr long labs(long);
// constexpr long long llabs(long long);

#include <cstdlib>
#include <cassert>
#include <type_traits>

#include "test_macros.h"

#ifndef __cpp_lib_constexpr_cmath
#  error "__cpp_lib_constexpr_cmath should be defined"
#endif

constexpr bool test() {
  auto a = std::div(7, -2);
  static_assert(std::is_same_v<decltype(a), std::div_t>);
  assert(a.quot == -3 && a.rem == 1);

  auto b = std::div(-7L, 2L);
  static_assert(std::is_same_v<decltype(b), std::ldiv_t>);
  assert(b.quot == -3 && b.rem == -1);

  auto c = std::div(7LL, 2LL);
  static_assert(std::is_same_v<decltype(c), std::lldiv_t>);
  assert(c.quot == 3 && c.rem == 1);

  auto d = std::ldiv(9L, 4L);
  assert(d.quot == 2 && d.rem == 1);

  auto e = std::lldiv(-9LL, 4LL);
  assert(e.quot == -2 && e.rem == -1);

  assert(std::labs(-3L) == 3L);
  assert(std::llabs(-3LL) == 3LL);
  return true;
}

int main(int, char**) {
  test();
  static_assert(test());
  return 0;
}
