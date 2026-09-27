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
#include <functional>

struct Value { int key; int source; };

template <class Policy>
void test(Policy&& policy) {
  std::array<int, 4> left{1, 2, 2, 5};
  std::array<int, 3> right{2, 3, 5};
  std::array<int, 5> output{};
  auto full = std::ranges::set_union(policy, left.begin(), left.end(), right.begin(), right.end(),
                                     output.begin(), output.end());
  assert(full.in1 == left.end() && full.in2 == right.end() && full.out == output.end());
  assert((output == std::array<int, 5>{1, 2, 2, 3, 5}));

  std::array<int, 2> short_output{};
  auto short_result = std::ranges::set_union(policy, left, right, short_output);
  assert(short_result.in1 == left.begin() + 2 && short_result.in2 == right.begin() + 1);
  assert(short_result.out == short_output.end());
  assert((short_output == std::array<int, 2>{1, 2}));

  std::array<Value, 2> projected_left{{{3, 1}, {1, 1}}};
  std::array<Value, 2> projected_right{{{3, 2}, {2, 2}}};
  std::array<Value, 3> projected_output{};
  auto projected = std::ranges::set_union(policy, projected_left, projected_right, projected_output,
                                           std::ranges::greater{}, &Value::key, &Value::key);
  assert(projected.in1 == projected_left.end() && projected.in2 == projected_right.end());
  assert(projected.out == projected_output.end());
  assert(projected_output[0].key == 3 && projected_output[0].source == 1);
  assert(projected_output[1].key == 2 && projected_output[1].source == 2);
  assert(projected_output[2].key == 1 && projected_output[2].source == 1);

  std::array<Value, 2> mixed_left{{{1, 100}, {3, 100}}};
  std::array<Value, 2> mixed_right{{{10, 2}, {20, 4}}};
  std::array<Value, 4> mixed_output{};
  auto mixed = std::ranges::set_union(policy, mixed_left, mixed_right, mixed_output,
                                       std::ranges::less{}, &Value::key, &Value::source);
  assert(mixed.in1 == mixed_left.end() && mixed.in2 == mixed_right.end() && mixed.out == mixed_output.end());
  assert(mixed_output[0].key == 1 && mixed_output[1].key == 10 &&
         mixed_output[2].key == 3 && mixed_output[3].key == 20);

  std::array<int, 0> empty{};
  auto none = std::ranges::set_union(policy, left, right, empty);
  assert(none.in1 == left.begin() && none.in2 == right.begin() && none.out == empty.end());
}

int main(int, char**) {
  test(std::execution::seq);
  test(std::execution::par);
  test(std::execution::par_unseq);
  test(std::execution::unseq);
}
