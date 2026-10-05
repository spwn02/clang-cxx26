//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: libcpp-has-no-incomplete-pstl

// [algorithms.parallel.overloads], copy_if: the execution-policy overloads of ranges::copy_if have a bounded output
// (iterator and sentinel, or output range) and process min(input size, output size) elements.

#include <algorithm>
#include <array>
#include <cassert>
#include <concepts>
#include <execution>
#include <functional>
#include <vector>

template <class... Args>
concept can_call = requires(Args&&... args) { std::ranges::copy_if(static_cast<Args&&>(args)...); };

struct Odd {
  bool operator()(int x) const { return x % 2 != 0; }
};

template <class P>
void test(P&& p) {
  using Pol = P&&;
  std::array<int, 7> a{1, 2, 3, 4, 5, 6, 7};
  // all four odd elements fit: the input is fully consumed
  std::array<int, 6> b{};
  auto r = std::ranges::copy_if(p, a.begin(), a.end(), b.begin(), b.end(), Odd{});
  assert(r.in == a.end() && r.out == b.begin() + 4);
  assert(b[0] == 1 && b[3] == 7 && b[4] == 0);
  // only two fit: copying stops at the first element that does not fit
  std::array<int, 2> c{};
  auto s = std::ranges::copy_if(p, a, c, Odd{});
  assert(s.in == a.begin() + 4 && s.out == c.end());
  assert(c[0] == 1 && c[1] == 3);
  // the output holds exactly the matching elements: the input is fully consumed
  std::array<int, 4> d{};
  auto t = std::ranges::copy_if(p, a, d, Odd{});
  assert(t.in == a.end() && t.out == d.end());
  // projections
  struct W {
    int v;
  };
  std::array<W, 3> w{{{1}, {2}, {3}}};
  std::array<W, 4> e{};
  auto u = std::ranges::copy_if(p, w, e, Odd{}, &W::v);
  assert(u.in == w.end() && u.out == e.begin() + 2 && e[1].v == 3);
  static_assert(!can_call<Pol, std::array<int, 7>&, int*, Odd>);
}

int main(int, char**) {
  test(std::execution::seq);
  test(std::execution::par);
  test(std::execution::par_unseq);
  test(std::execution::unseq);
  return 0;
}
