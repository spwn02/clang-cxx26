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
  std::array<int, 5> a{1, 2, 1, 3, 1};
  auto e = std::ranges::remove(p, a, 1);
  assert(e.begin() == a.begin() + 2 && e.end() == a.end());
  assert(a[0] == 2 && a[1] == 3);

  std::array<int, 5> b{1, 2, 1, 3, 1};
  auto f = std::remove(p, b.begin(), b.end(), 3);
  assert(f == b.end() - 1);

  // Non-default projection must actually be applied, not silently discarded.
  struct wrapper {
    int value;
  };
  std::array<wrapper, 4> c{{{1}, {2}, {1}, {3}}};
  auto g = std::ranges::remove(p, c, 1, &wrapper::value);
  assert(g.begin() == c.begin() + 2 && g.end() == c.end());
  assert(c[0].value == 2 && c[1].value == 3);
}
int main(int, char**) { test(std::execution::seq); test(std::execution::par); test(std::execution::par_unseq); test(std::execution::unseq); }
