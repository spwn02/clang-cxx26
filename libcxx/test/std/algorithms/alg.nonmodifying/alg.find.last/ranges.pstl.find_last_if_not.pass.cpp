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
  std::array<int, 4> a{2, 4, 6, 7};
  auto r = std::ranges::find_last_if_not(p, a, [](int x) { return x % 2 == 0; });
  assert(r.begin() == a.begin() + 3 && r.end() == a.end());
  std::array<int, 4> all_even{2, 4, 6, 8};
  auto not_found = std::ranges::find_last_if_not(p, all_even, [](int x) { return x % 2 == 0; });
  assert(not_found.begin() == all_even.end() && not_found.end() == all_even.end());
}
int main(int, char**) { test(std::execution::seq); test(std::execution::par); test(std::execution::par_unseq); test(std::execution::unseq); }
