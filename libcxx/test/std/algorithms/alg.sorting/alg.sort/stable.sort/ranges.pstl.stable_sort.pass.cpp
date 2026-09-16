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

struct Item { int key; int order; };

template <class P> void test(P&& p) {
  std::array<Item, 4> a{{{2, 0}, {1, 1}, {2, 2}, {1, 3}}};
  assert(std::ranges::stable_sort(p, a.begin(), a.end(), std::ranges::less{}, &Item::key) == a.end());
  assert((a[0].key == 1 && a[0].order == 1 && a[1].key == 1 && a[1].order == 3));
  assert(std::ranges::stable_sort(p, a, std::ranges::greater{}, &Item::key) == a.end());
  assert((a[0].key == 2 && a[0].order == 0 && a[1].key == 2 && a[1].order == 2));
}

int main(int, char**) {
  test(std::execution::seq);
  test(std::execution::par);
  test(std::execution::par_unseq);
  test(std::execution::unseq);
}
