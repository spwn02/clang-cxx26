//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: libcpp-has-no-incomplete-pstl

// [algorithms.parallel.overloads], reverse_copy: the execution-policy overloads of ranges::reverse_copy have a bounded output
// (iterator and sentinel, or output range) and process min(input size, output size) elements.

#include <algorithm>
#include <array>
#include <cassert>
#include <concepts>
#include <execution>
#include <functional>
#include <vector>

template <class... Args>
concept can_call = requires(Args&&... args) { std::ranges::reverse_copy(static_cast<Args&&>(args)...); };

template <class P>
void test(P&& p) {
  using Pol = P&&;
  std::array<int, 5> a{1, 2, 3, 4, 5};
  // the whole input fits
  std::array<int, 7> b{};
  auto r = std::ranges::reverse_copy(p, a.begin(), a.end(), b.begin(), b.end());
  assert(r.in1 == a.end() && r.in2 == a.begin() && r.out == b.begin() + 5);
  assert(b[0] == 5 && b[4] == 1 && b[5] == 0);
  // only the last N elements are copied, reversed
  std::array<int, 2> c{};
  auto q = std::ranges::reverse_copy(p, a, c);
  assert(q.in1 == a.end() && q.in2 == a.begin() + 3 && q.out == c.end());
  assert(c[0] == 5 && c[1] == 4);
  static_assert(std::same_as<std::ranges::reverse_copy_truncated_result<int*, long*>,
                             std::ranges::in_in_out_result<int*, int*, long*>>);
  static_assert(!can_call<Pol, std::array<int, 5>&, int*>);
}

int main(int, char**) {
  test(std::execution::seq);
  test(std::execution::par);
  test(std::execution::par_unseq);
  test(std::execution::unseq);
  return 0;
}
