//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: libcpp-has-no-incomplete-pstl

// [algorithms.parallel.overloads], replace_copy_if: the execution-policy overloads of ranges::replace_copy_if have a bounded output
// (iterator and sentinel, or output range) and process min(input size, output size) elements.

#include <algorithm>
#include <array>
#include <cassert>
#include <concepts>
#include <execution>
#include <functional>
#include <vector>

template <class... Args>
concept can_call = requires(Args&&... args) { std::ranges::replace_copy_if(static_cast<Args&&>(args)...); };

struct IsOne {
  bool operator()(int x) const { return x == 1; }
};

template <class P>
void test(P&& p) {
  using Pol = P&&;
  std::array<int, 5> a{1, 2, 1, 3, 1};
  std::array<int, 2> b{};
  auto r = std::ranges::replace_copy_if(p, a.begin(), a.end(), b.begin(), b.end(), IsOne{}, 9);
  assert(r.in == a.begin() + 2 && r.out == b.end());
  assert(b[0] == 9 && b[1] == 2);
  std::array<int, 8> c{};
  auto q = std::ranges::replace_copy_if(p, a, c, IsOne{}, 0);
  assert(q.in == a.end() && q.out == c.begin() + 5);
  assert(c[0] == 0 && c[1] == 2 && c[3] == 3 && c[4] == 0);
  static_assert(!can_call<Pol, std::array<int, 5>&, int*, IsOne, int>);
}

int main(int, char**) {
  test(std::execution::seq);
  test(std::execution::par);
  test(std::execution::par_unseq);
  test(std::execution::unseq);
  return 0;
}
