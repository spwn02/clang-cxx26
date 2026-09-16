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
  std::array<int, 5> a{1, 2, 3, 4, 5};
  std::array<int, 3> b{2, 3, 4};
  std::array<int, 3> c{2, 3, 6};
  assert(std::includes(p, a.begin(), a.end(), b.begin(), b.end()));
  assert(std::ranges::includes(p, a, b));
  assert(!std::includes(p, a.begin(), a.end(), c.begin(), c.end()));
  assert(!std::ranges::includes(p, a, c));
}
int main(int, char**) { test(std::execution::seq); test(std::execution::par); test(std::execution::par_unseq); test(std::execution::unseq); }
