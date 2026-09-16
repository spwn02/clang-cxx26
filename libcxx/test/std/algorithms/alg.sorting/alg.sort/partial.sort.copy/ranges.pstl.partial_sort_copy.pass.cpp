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
  std::array<int, 5> in{5, 1, 4, 2, 3};
  std::array<int, 3> out{};
  auto r = std::ranges::partial_sort_copy(p, in, out);
  assert(r.in == in.end() && r.out == out.end());
  assert((out == std::array<int, 3>{1, 2, 3}));
  std::array<int, 3> out2{};
  assert(std::partial_sort_copy(p, in.begin(), in.end(), out2.begin(), out2.end()) == out2.end());
}
int main(int, char**) { test(std::execution::seq); test(std::execution::par); test(std::execution::par_unseq); test(std::execution::unseq); }
