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
  std::array<int, 4> a{1, 2, 7, 4};
  std::array<int, 3> b{1, 2, 3};
  auto x = std::mismatch(p, a.begin(), a.end(), b.begin(), b.end(), std::equal_to{});
  assert(x.first == a.begin() + 2 && x.second == b.begin() + 2);
  auto y = std::ranges::mismatch(p, a, b);
  assert(y.in1 == a.begin() + 2 && y.in2 == b.begin() + 2);
  std::array<int, 2> c{1, 2};
  y = std::ranges::mismatch(p, c, b);
  assert(y.in1 == c.end() && y.in2 == b.begin() + 2);
}
int main(int, char**) { test(std::execution::seq); test(std::execution::par); test(std::execution::par_unseq); test(std::execution::unseq); }
