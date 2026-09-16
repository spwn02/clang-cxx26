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
  std::array<int, 6> a{1, 3, 5, 2, 4, 6};
  assert(std::ranges::inplace_merge(p, a, a.begin() + 3) == a.end());
  assert((a == std::array<int, 6>{1, 2, 3, 4, 5, 6}));
  std::inplace_merge(p, a.begin(), a.begin() + 3, a.end());
}
int main(int, char**) { test(std::execution::seq); test(std::execution::par); test(std::execution::par_unseq); test(std::execution::unseq); }
