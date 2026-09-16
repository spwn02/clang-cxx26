//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: libcpp-has-no-incomplete-pstl
#include <algorithm>
#include <array>
#include <cassert>
#include <execution>

template <class P> void test(P&& p) {
  std::array<int, 5> a{4, 3, 2, 1, 0};
  std::array<int, 2> choices{8, 2};
  assert(std::find_first_of(p, a.begin(), a.end(), choices.begin(), choices.end(), std::equal_to{}) == a.begin() + 2);
  assert(std::ranges::find_first_of(p, a, choices) == a.begin() + 2);
  std::array<int, 1> absent{9};
  assert(std::ranges::find_first_of(p, a, absent) == a.end());
}
int main(int, char**) { test(std::execution::seq); test(std::execution::par); test(std::execution::par_unseq); test(std::execution::unseq); }
