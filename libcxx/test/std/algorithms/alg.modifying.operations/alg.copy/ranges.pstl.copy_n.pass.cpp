//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: libcpp-has-no-incomplete-pstl

// [algorithms.parallel.overloads], copy_n: the execution-policy overloads of ranges::copy_n have a bounded output
// (iterator and sentinel, or output range) and process min(input size, output size) elements.

#include <algorithm>
#include <array>
#include <cassert>
#include <concepts>
#include <execution>
#include <functional>
#include <vector>

template <class... Args>
concept can_call = requires(Args&&... args) { std::ranges::copy_n(static_cast<Args&&>(args)...); };

template <class P>
void test(P&& p) {
  using Pol = P&&;
  std::array<int, 4> a{1, 2, 3, 4};
  std::array<int, 6> b{};
  auto r = std::ranges::copy_n(p, a.begin(), 4, b.begin(), b.begin() + 3);
  assert(r.in == a.begin() + 3 && r.out == b.begin() + 3);
  assert(b[2] == 3 && b[3] == 0);
  auto q = std::ranges::copy_n(p, a.begin(), 2, b.begin(), b.end());
  assert(q.in == a.begin() + 2 && q.out == b.begin() + 2);
  // a negative count copies nothing
  auto s = std::ranges::copy_n(p, a.begin(), -3, b.begin(), b.end());
  assert(s.in == a.begin() && s.out == b.begin());
  static_assert(!can_call<Pol, int*, int, int*>);
  static_assert(can_call<Pol, int*, int, int*, int*>);
}

int main(int, char**) {
  test(std::execution::seq);
  test(std::execution::par);
  test(std::execution::par_unseq);
  test(std::execution::unseq);
  return 0;
}
