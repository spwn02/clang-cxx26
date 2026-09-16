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
  std::array<int, 6> a{0, 1, 2, 1, 2, 3};
  std::array<int, 2> pat{1, 2};
  auto r = std::ranges::search(p, a, pat);
  assert(r.begin() == a.begin() + 1 && r.end() == a.begin() + 3);
  std::array<int, 7> too_long{0, 1, 2, 3, 4, 5, 6};
  assert(std::ranges::search(p, a, too_long).begin() == a.end());
  std::array<int, 0> empty{};
  assert(std::ranges::search(p, a, empty).begin() == a.begin());
}
int main(int, char**) { test(std::execution::seq); test(std::execution::par); test(std::execution::par_unseq); test(std::execution::unseq); }
