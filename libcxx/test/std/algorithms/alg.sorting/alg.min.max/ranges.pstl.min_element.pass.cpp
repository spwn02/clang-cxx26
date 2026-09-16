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
template <class P>
void test(P&& p) {
  std::array<int, 5> a{4, 1, 3, 1, 2};
  assert(std::min_element(p, a.begin(), a.end()) == a.begin() + 1);
  assert(std::ranges::min_element(p, a.begin(), a.end()) == a.begin() + 1);
  assert(std::ranges::min_element(p, a) == a.begin() + 1);
}
int main(int, char**) {
  test(std::execution::seq);
  test(std::execution::par);
  test(std::execution::par_unseq);
  test(std::execution::unseq);
}
