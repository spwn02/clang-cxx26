//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: libcpp-has-no-incomplete-pstl

// [algorithms.parallel.overloads], copy: the execution-policy overloads of ranges::copy have a bounded output
// (iterator and sentinel, or output range) and process min(input size, output size) elements.

#include <algorithm>
#include <array>
#include <cassert>
#include <concepts>
#include <execution>
#include <functional>
#include <vector>

template <class... Args>
concept can_call = requires(Args&&... args) { std::ranges::copy(static_cast<Args&&>(args)...); };

template <class P>
void test(P&& p) {
  using Pol = P&&;
  std::array<int, 4> a{1, 2, 3, 4};
  // truncated output: two elements are copied
  std::array<int, 6> b{};
  auto r = std::ranges::copy(p, a.begin(), a.end(), b.begin(), b.begin() + 2);
  assert(r.in == a.begin() + 2 && r.out == b.begin() + 2);
  assert(b[0] == 1 && b[1] == 2 && b[2] == 0);
  // output larger than the input: all elements are copied
  auto q = std::ranges::copy(p, a, b);
  assert(q.in == a.end() && q.out == b.begin() + 4);
  assert(b[3] == 4 && b[4] == 0);
  // output range smaller than the input
  std::array<int, 3> c{};
  auto s = std::ranges::copy(p, a, c);
  assert(s.in == a.begin() + 3 && s.out == c.end());
  assert(c[2] == 3);
  // empty output
  std::vector<int> none;
  auto t = std::ranges::copy(p, a, none);
  assert(t.in == a.begin() && t.out == none.end());
  // the execution-policy overloads have a bounded output only
  static_assert(!can_call<Pol, std::array<int, 4>&, int*>);
  static_assert(can_call<Pol, std::array<int, 4>&, std::array<int, 4>&>);
}

int main(int, char**) {
  test(std::execution::seq);
  test(std::execution::par);
  test(std::execution::par_unseq);
  test(std::execution::unseq);
  return 0;
}
