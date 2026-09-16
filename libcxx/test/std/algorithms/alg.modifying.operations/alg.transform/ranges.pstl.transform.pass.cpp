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
template <class P> void test(P&& p) {
  std::array<int, 4> a{1, 2, 3, 4}, b{4, 3, 2, 1}, out{};
  auto r = std::ranges::transform(p, a.begin(), a.end(), out.begin(), [](int x) { return x * 2; });
  assert(r.in == a.end() && r.out == out.end() && out[3] == 8);
  auto q = std::ranges::transform(p, a, b, out.begin(), [](int x, int y) { return x + y; });
  assert(q.in1 == a.end() && q.in2 == b.end() && q.out == out.end() && out[0] == 5);

  // Binary transform with a longer second range: `in2` must reflect how far range2 was
  // actually walked (bounded by the shorter range1), not range2's own full end.
  std::array<int, 3> short_a{1, 2, 3};
  std::array<int, 5> long_b{10, 20, 30, 40, 50};
  std::array<int, 3> out2{};
  auto s = std::ranges::transform(p, short_a, long_b, out2.begin(), [](int x, int y) { return x + y; });
  assert(s.in1 == short_a.end());
  assert(s.in2 == long_b.begin() + 3);
  assert(s.out == out2.end());
  assert(out2[0] == 11 && out2[1] == 22 && out2[2] == 33);
}
int main(int, char**) { test(std::execution::seq); test(std::execution::par); test(std::execution::par_unseq); test(std::execution::unseq); }
