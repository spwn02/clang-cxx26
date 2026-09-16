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

struct Item { int key; };

template <class P> void test(P&& p) {
  std::array<Item, 4> a{{{3}, {1}, {4}, {2}}};
  assert(std::ranges::sort(p, a.begin(), a.end(), std::ranges::less{}, &Item::key) == a.end());
  assert((a[0].key == 1 && a[1].key == 2 && a[2].key == 3 && a[3].key == 4));
  assert(std::ranges::sort(p, a, std::ranges::greater{}, &Item::key) == a.end());
  assert((a[0].key == 4 && a[1].key == 3 && a[2].key == 2 && a[3].key == 1));
}

int main(int, char**) {
  test(std::execution::seq);
  test(std::execution::par);
  test(std::execution::par_unseq);
  test(std::execution::unseq);
}
