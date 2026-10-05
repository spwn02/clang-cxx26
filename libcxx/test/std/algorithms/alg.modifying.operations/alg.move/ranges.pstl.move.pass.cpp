//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: libcpp-has-no-incomplete-pstl

// [algorithms.parallel.overloads], move: the execution-policy overloads of ranges::move have a bounded output
// (iterator and sentinel, or output range) and process min(input size, output size) elements.

#include <algorithm>
#include <array>
#include <cassert>
#include <concepts>
#include <execution>
#include <functional>
#include <vector>

template <class... Args>
concept can_call = requires(Args&&... args) { std::ranges::move(static_cast<Args&&>(args)...); };

struct MoveOnly {
  int value = 0;
  MoveOnly() = default;
  explicit MoveOnly(int v) : value(v) {}
  MoveOnly(MoveOnly&& o) : value(o.value) { o.value = -1; }
  MoveOnly& operator=(MoveOnly&& o) { value = o.value; o.value = -1; return *this; }
};

template <class P>
void test(P&& p) {
  using Pol = P&&;
  std::vector<MoveOnly> a;
  for (int i = 1; i <= 4; ++i)
    a.emplace_back(i);
  std::vector<MoveOnly> b(2);
  auto r = std::ranges::move(p, a.begin(), a.end(), b.begin(), b.end());
  assert(r.in == a.begin() + 2 && r.out == b.end());
  assert(b[0].value == 1 && b[1].value == 2 && a[0].value == -1 && a[2].value == 3);
  std::vector<MoveOnly> c(6);
  auto q = std::ranges::move(p, a, c);
  assert(q.in == a.end() && q.out == c.begin() + 4);
  assert(c[2].value == 3 && c[3].value == 4);
  static_assert(!can_call<Pol, std::vector<MoveOnly>&, MoveOnly*>);
}

int main(int, char**) {
  test(std::execution::seq);
  test(std::execution::par);
  test(std::execution::par_unseq);
  test(std::execution::unseq);
  return 0;
}
