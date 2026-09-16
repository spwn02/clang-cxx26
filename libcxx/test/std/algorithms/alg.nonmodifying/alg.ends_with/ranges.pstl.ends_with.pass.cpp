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
  std::array<int, 4> a{1, 2, 3, 4};
  std::array<int, 2> suffix{3, 4};
  assert(std::ranges::ends_with(p, a, suffix));
  std::array<int, 5> too_long{0, 1, 2, 3, 4};
  assert(!std::ranges::ends_with(p, a, too_long));
  std::array<int, 0> empty{};
  assert(std::ranges::ends_with(p, a, empty));
}
int main(int, char**) { test(std::execution::seq); test(std::execution::par); test(std::execution::par_unseq); test(std::execution::unseq); }
