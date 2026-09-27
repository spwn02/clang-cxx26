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

struct Value { int key; int source; };

template <class Policy>
void test(Policy&& policy) {
  std::array<int, 5> left{1, 2, 2, 4, 6};
  std::array<int, 3> right{2, 3, 6};
  std::array<int, 3> output{};
  auto full = std::ranges::set_difference(policy, left.begin(), left.end(), right.begin(), right.end(),
                                          output.begin(), output.end());
  assert(full.in == left.end() && full.out == output.end());
  assert((output == std::array<int, 3>{1, 2, 4}));

  std::array<int, 1> short_output{};
  auto partial = std::ranges::set_difference(policy, left, right, short_output);
  assert(partial.in == left.begin() + 2 && partial.out == short_output.end());
  assert(short_output[0] == 1);

  std::array<int, 0> empty{};
  auto none = std::ranges::set_difference(policy, left, right, empty);
  assert(none.in == left.begin() && none.out == empty.end());

  std::array<Value, 3> projected_left{{{5, 1}, {3, 1}, {1, 1}}};
  std::array<Value, 2> projected_right{{{3, 2}, {2, 2}}};
  std::array<Value, 2> projected_output{};
  auto projected = std::ranges::set_difference(policy, projected_left, projected_right, projected_output,
                                                std::ranges::greater{}, &Value::key, &Value::key);
  assert(projected.in == projected_left.end() && projected.out == projected_output.end());
  assert(projected_output[0].key == 5 && projected_output[1].key == 1);
  assert(projected_output[0].source == 1 && projected_output[1].source == 1);
}

int main(int, char**) {
  test(std::execution::seq);
  test(std::execution::par);
  test(std::execution::par_unseq);
  test(std::execution::unseq);
}
