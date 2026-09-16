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
  std::array<int, 5> a{1, 2, 2, 3, 4};
  assert(std::adjacent_find(p, a.begin(), a.end(), std::equal_to{}) == a.begin() + 1);
  assert(std::ranges::adjacent_find(p, a, std::equal_to{}) == a.begin() + 1);
  std::array<int, 2> b{1, 2};
  assert(std::ranges::adjacent_find(p, b, std::equal_to{}) == b.end());
}
int main(int, char**) { test(std::execution::seq); test(std::execution::par); test(std::execution::par_unseq); test(std::execution::unseq); }
