//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: libcpp-has-no-incomplete-pstl

// [algorithms.parallel.overloads], replace_copy: the execution-policy overloads of ranges::replace_copy have a bounded output
// (iterator and sentinel, or output range) and process min(input size, output size) elements.

#include <algorithm>
#include <array>
#include <cassert>
#include <concepts>
#include <execution>
#include <functional>
#include <vector>

template <class... Args>
concept can_call = requires(Args&&... args) { std::ranges::replace_copy(static_cast<Args&&>(args)...); };

template <class P>
void test(P&& p) {
  using Pol = P&&;
  std::array<int, 5> a{1, 2, 1, 3, 1};
  std::array<int, 3> b{};
  auto r = std::ranges::replace_copy(p, a.begin(), a.end(), b.begin(), b.end(), 1, 9);
  assert(r.in == a.begin() + 3 && r.out == b.end());
  assert(b[0] == 9 && b[1] == 2 && b[2] == 9);
  std::array<int, 7> c{};
  auto q = std::ranges::replace_copy(p, a, c, 2, 8);
  assert(q.in == a.end() && q.out == c.begin() + 5);
  assert(c[0] == 1 && c[1] == 8 && c[4] == 1 && c[5] == 0);
  // the projection selects what is compared
  struct W {
    int v;
  };
  std::array<W, 3> w{{{1}, {2}, {1}}};
  std::array<W, 3> d{};
  std::ranges::replace_copy(p, w, d, 1, W{7}, &W::v);
  assert(d[0].v == 7 && d[1].v == 2 && d[2].v == 7);
  static_assert(!can_call<Pol, std::array<int, 5>&, int*, int, int>);
}

int main(int, char**) {
  test(std::execution::seq);
  test(std::execution::par);
  test(std::execution::par_unseq);
  test(std::execution::unseq);
  return 0;
}
