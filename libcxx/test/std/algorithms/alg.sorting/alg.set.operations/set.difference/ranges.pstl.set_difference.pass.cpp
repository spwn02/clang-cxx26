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
  std::array<int, 4> a{1, 2, 3, 5}, b{2, 4, 5}; std::array<int, 4> out{};
  assert(std::set_difference(p, a.begin(), a.end(), b.begin(), b.end(), out.begin()) == out.begin() + 2);
  assert((out == std::array<int, 4>{1, 3, 0, 0}));
  std::array<int, 4> c{1, 2, 3, 5}, d{2, 4, 5}; std::array<int, 4> out2{};
  auto r = std::ranges::set_difference(p, c, d, out2.begin());
  assert(r.in1 == c.end() && r.in2 == d.end() && r.out == out2.begin() + 2);
  assert((out2 == std::array<int, 4>{1, 3, 0, 0}));
}
int main(int, char**) { test(std::execution::seq); test(std::execution::par); test(std::execution::par_unseq); test(std::execution::unseq); }
