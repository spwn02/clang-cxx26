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
  std::array<int, 7> input{1, 1, 2, 2, 2, 3, 3};
  std::array<int, 3> output{};
  auto full = std::ranges::unique_copy(policy, input.begin(), input.end(), output.begin(), output.end());
  assert(full.in == input.end() && full.out == output.end());
  assert((output == std::array<int, 3>{1, 2, 3}));

  std::array<int, 2> short_output{};
  auto partial = std::ranges::unique_copy(policy, input, short_output);
  assert(partial.in == input.begin() + 5 && partial.out == short_output.end());
  assert((short_output == std::array<int, 2>{1, 2}));

  std::array<int, 0> empty{};
  auto none = std::ranges::unique_copy(policy, input, empty);
  assert(none.in == input.begin() && none.out == empty.end());

  std::array<Value, 5> projected_input{{{3, 1}, {3, 2}, {2, 3}, {1, 4}, {1, 5}}};
  std::array<Value, 3> projected_output{};
  auto projected = std::ranges::unique_copy(policy, projected_input, projected_output,
                                             std::ranges::equal_to{}, &Value::key);
  assert(projected.in == projected_input.end() && projected.out == projected_output.end());
  assert(projected_output[0].source == 1 && projected_output[1].source == 3 && projected_output[2].source == 4);

  std::array<int, 5> descending{5, 4, 3, 2, 1};
  std::array<int, 2> grouped{};
  auto grouped_result = std::ranges::unique_copy(policy, descending, grouped,
      [](int current, int previous) { return current / 2 == previous / 2; });
  assert(grouped_result.in == descending.begin() + 4 && grouped_result.out == grouped.end());
  assert((grouped == std::array<int, 2>{5, 3}));
}

int main(int, char**) {
  test(std::execution::seq);
  test(std::execution::par);
  test(std::execution::par_unseq);
  test(std::execution::unseq);
}
