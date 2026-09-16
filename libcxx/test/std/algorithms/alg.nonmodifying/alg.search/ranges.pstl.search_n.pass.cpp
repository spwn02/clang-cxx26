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
  std::array<int, 5> a{1, 2, 2, 2, 3};
  auto r = std::ranges::search_n(p, a, 3, 2);
  assert(r.begin() == a.begin() + 1 && r.end() == a.begin() + 4); // found run spans exactly [1, 4), not to a.end()
  auto not_found = std::ranges::search_n(p, a, 4, 2);
  assert(not_found.begin() == a.end() && not_found.end() == a.end());
}
int main(int, char**) { test(std::execution::seq); test(std::execution::par); test(std::execution::par_unseq); test(std::execution::unseq); }
