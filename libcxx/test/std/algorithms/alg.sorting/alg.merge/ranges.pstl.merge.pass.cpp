//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: libcpp-has-no-incomplete-pstl

// [algorithms.parallel.overloads], merge: the execution-policy overloads of ranges::merge have a bounded output
// (iterator and sentinel, or output range) and process min(input size, output size) elements.

#include <algorithm>
#include <array>
#include <cassert>
#include <concepts>
#include <execution>
#include <functional>
#include <vector>

template <class... Args>
concept can_call = requires(Args&&... args) { std::ranges::merge(static_cast<Args&&>(args)...); };

template <class P>
void test(P&& p) {
  using Pol = P&&;
  std::array<int, 4> a{1, 3, 5, 7};
  std::array<int, 3> b{2, 4, 6};
  std::array<int, 8> c{};
  auto r = std::ranges::merge(p, a.begin(), a.end(), b.begin(), b.end(), c.begin(), c.end());
  assert(r.in1 == a.end() && r.in2 == b.end() && r.out == c.begin() + 7);
  assert(c[0] == 1 && c[3] == 4 && c[6] == 7 && c[7] == 0);
  // only the first five merged elements fit: 1 2 3 4 5 take three from a and two from b
  std::array<int, 5> d{};
  auto q = std::ranges::merge(p, a, b, d);
  assert(q.in1 == a.begin() + 3 && q.in2 == b.begin() + 2 && q.out == d.end());
  assert(d[4] == 5);
  // a custom comparison and projections
  std::array<int, 3> e{5, 3, 1};
  std::array<int, 2> f{4, 2};
  std::array<int, 3> g{};
  auto s = std::ranges::merge(p, e, f, g, std::greater<>{});
  assert(s.in1 == e.begin() + 2 && s.in2 == f.begin() + 1 && g[0] == 5 && g[1] == 4 && g[2] == 3);
  static_assert(!can_call<Pol, std::array<int, 4>&, std::array<int, 3>&, int*>);
}

int main(int, char**) {
  test(std::execution::seq);
  test(std::execution::par);
  test(std::execution::par_unseq);
  test(std::execution::unseq);
  return 0;
}
