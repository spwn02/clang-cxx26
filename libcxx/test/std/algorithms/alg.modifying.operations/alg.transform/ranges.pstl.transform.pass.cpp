//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: libcpp-has-no-incomplete-pstl

// [algorithms.parallel.overloads], transform: the execution-policy overloads of ranges::transform have a bounded output
// (iterator and sentinel, or output range) and process min(input size, output size) elements.

#include <algorithm>
#include <array>
#include <cassert>
#include <concepts>
#include <execution>
#include <functional>
#include <vector>

template <class... Args>
concept can_call = requires(Args&&... args) { std::ranges::transform(static_cast<Args&&>(args)...); };

struct Twice {
  int operator()(int x) const { return 2 * x; }
};

template <class P>
void test(P&& p) {
  using Pol = P&&;
  std::array<int, 4> a{1, 2, 3, 4};
  std::array<int, 3> b{};
  auto r = std::ranges::transform(p, a.begin(), a.end(), b.begin(), b.end(), Twice{});
  assert(r.in == a.begin() + 3 && r.out == b.end());
  assert(b[0] == 2 && b[2] == 6);
  std::array<int, 6> c{};
  auto q = std::ranges::transform(p, a, c, Twice{});
  assert(q.in == a.end() && q.out == c.begin() + 4 && c[3] == 8 && c[4] == 0);
  // binary: the shortest of the three bounds decides
  std::array<int, 4> x{1, 2, 3, 4};
  std::array<int, 3> y{10, 20, 30};
  std::array<int, 5> z{};
  auto s = std::ranges::transform(p, x, y, z, std::plus<>{});
  assert(s.in1 == x.begin() + 3 && s.in2 == y.end() && s.out == z.begin() + 3);
  assert(z[0] == 11 && z[2] == 33 && z[3] == 0);
  std::array<int, 2> small{};
  auto t = std::ranges::transform(p, x.begin(), x.end(), y.begin(), y.end(), small.begin(), small.end(), std::plus<>{});
  assert(t.in1 == x.begin() + 2 && t.in2 == y.begin() + 2 && t.out == small.end());
  // projections
  struct W {
    int v;
  };
  std::array<W, 3> w{{{1}, {2}, {3}}};
  std::array<int, 3> u{};
  std::ranges::transform(p, w, u, Twice{}, &W::v);
  assert(u[2] == 6);
  static_assert(!can_call<Pol, std::array<int, 4>&, int*, Twice>);
}

int main(int, char**) {
  test(std::execution::seq);
  test(std::execution::par);
  test(std::execution::par_unseq);
  test(std::execution::unseq);
  return 0;
}
