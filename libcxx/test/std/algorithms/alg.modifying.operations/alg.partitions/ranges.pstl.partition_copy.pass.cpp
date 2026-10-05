//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: libcpp-has-no-incomplete-pstl

// [algorithms.parallel.overloads], partition_copy: the execution-policy overloads of ranges::partition_copy have a bounded output
// (iterator and sentinel, or output range) and process min(input size, output size) elements.

#include <algorithm>
#include <array>
#include <cassert>
#include <concepts>
#include <execution>
#include <functional>
#include <vector>

template <class... Args>
concept can_call = requires(Args&&... args) { std::ranges::partition_copy(static_cast<Args&&>(args)...); };

struct Even {
  bool operator()(int x) const { return x % 2 == 0; }
};

template <class P>
void test(P&& p) {
  using Pol = P&&;
  std::array<int, 6> a{1, 2, 3, 4, 5, 6};
  std::array<int, 4> t{}, f{};
  auto r = std::ranges::partition_copy(p, a.begin(), a.end(), t.begin(), t.end(), f.begin(), f.end(), Even{});
  assert(r.in == a.end() && r.out1 == t.begin() + 3 && r.out2 == f.begin() + 3);
  assert(t[0] == 2 && t[2] == 6 && f[0] == 1 && f[2] == 5);
  // the true output fills up first: copying stops at the first element that does not fit
  std::array<int, 2> t2{};
  std::array<int, 6> f2{};
  auto q = std::ranges::partition_copy(p, a, t2, f2, [](int x) { return x > 2; });
  assert(q.in == a.begin() + 4 && q.out1 == t2.end() && q.out2 == f2.begin() + 2);
  assert(t2[0] == 3 && t2[1] == 4 && f2[0] == 1 && f2[1] == 2);
  // the false output fills up first
  std::array<int, 6> t3{};
  std::array<int, 1> f3{};
  auto s = std::ranges::partition_copy(p, a, t3, f3, Even{});
  assert(s.in == a.begin() + 2 && s.out1 == t3.begin() + 1 && s.out2 == f3.end());
  static_assert(!can_call<Pol, std::array<int, 6>&, int*, int*, Even>);
}

int main(int, char**) {
  test(std::execution::seq);
  test(std::execution::par);
  test(std::execution::par_unseq);
  test(std::execution::unseq);
  return 0;
}
