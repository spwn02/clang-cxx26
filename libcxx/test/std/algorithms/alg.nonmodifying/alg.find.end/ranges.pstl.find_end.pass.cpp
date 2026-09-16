//===----------------------------------------------------------------------===//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: libcpp-has-no-incomplete-pstl
#include <algorithm>
#include <array>
#include <cassert>
#include <execution>

template <class P> void test(P&& p) {
  std::array<int, 6> a{1, 2, 1, 2, 1, 3};
  std::array<int, 2> pat{1, 2};
  assert(std::find_end(p, a.begin(), a.end(), pat.begin(), pat.end(), std::equal_to{}) == a.begin() + 2);
  auto r = std::ranges::find_end(p, a, pat);
  assert(r.begin() == a.begin() + 2 && r.end() == a.begin() + 4);
  std::array<int, 7> too_long{0, 1, 2, 3, 4, 5, 6};
  assert(std::ranges::find_end(p, a, too_long).begin() == a.end());
  std::array<int, 0> empty{};
  assert(std::ranges::find_end(p, a, empty).begin() == a.end());
}
int main(int, char**) { test(std::execution::seq); test(std::execution::par); test(std::execution::par_unseq); test(std::execution::unseq); }
