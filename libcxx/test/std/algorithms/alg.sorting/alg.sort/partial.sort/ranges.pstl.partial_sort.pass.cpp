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
  std::array<int, 5> a{5, 1, 4, 2, 3};
  assert(std::ranges::partial_sort(p, a.begin(), a.begin() + 3, a.end()) == a.end());
  assert((a[0] == 1 && a[1] == 2 && a[2] == 3));
  std::partial_sort(p, a.begin(), a.begin() + 2, a.end());
  assert((a[0] == 1 && a[1] == 2));
}
int main(int, char**) { test(std::execution::seq); test(std::execution::par); test(std::execution::par_unseq); test(std::execution::unseq); }
