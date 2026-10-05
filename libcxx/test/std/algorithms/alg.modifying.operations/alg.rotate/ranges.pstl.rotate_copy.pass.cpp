//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: libcpp-has-no-incomplete-pstl

// [algorithms.parallel.overloads], rotate_copy: the execution-policy overloads of ranges::rotate_copy have a bounded output
// (iterator and sentinel, or output range) and process min(input size, output size) elements.

#include <algorithm>
#include <array>
#include <cassert>
#include <concepts>
#include <execution>
#include <functional>
#include <vector>

template <class... Args>
concept can_call = requires(Args&&... args) { std::ranges::rotate_copy(static_cast<Args&&>(args)...); };

template <class P>
void test(P&& p) {
  using Pol = P&&;
  std::array<int, 5> a{1, 2, 3, 4, 5};
  // the whole input fits
  std::array<int, 7> b{};
  auto r = std::ranges::rotate_copy(p, a.begin(), a.begin() + 2, a.end(), b.begin(), b.end());
  assert(r.in1 == a.end() && r.in2 == a.begin() + 2 && r.out == b.begin() + 5);
  assert(b[0] == 3 && b[2] == 5 && b[3] == 1 && b[4] == 2 && b[5] == 0);
  // fewer than last - middle elements fit: only part of [middle, last) is copied
  std::array<int, 2> c{};
  auto q = std::ranges::rotate_copy(p, a, a.begin() + 2, c);
  assert(q.in1 == a.begin() + 4 && q.in2 == a.begin() && q.out == c.end());
  assert(c[0] == 3 && c[1] == 4);
  // more than last - middle but not all fit
  std::array<int, 4> d{};
  auto s = std::ranges::rotate_copy(p, a, a.begin() + 2, d);
  assert(s.in1 == a.end() && s.in2 == a.begin() + 1 && s.out == d.end());
  assert(d[2] == 5 && d[3] == 1);
  static_assert(std::same_as<std::ranges::rotate_copy_truncated_result<int*, long*>,
                             std::ranges::in_in_out_result<int*, int*, long*>>);
  static_assert(!can_call<Pol, std::array<int, 5>&, int*, int*>);
}

int main(int, char**) {
  test(std::execution::seq);
  test(std::execution::par);
  test(std::execution::par_unseq);
  test(std::execution::unseq);
  return 0;
}
