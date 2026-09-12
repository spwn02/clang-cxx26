//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: libcpp-has-no-incomplete-pstl

// P3179R9: ranges execution-policy overloads for all_of, any_of, and none_of.

#include <algorithm>
#include <array>
#include <cassert>
#include <execution>
#include <forward_list>

template <class Policy>
void test(Policy&& policy) {
  struct Item {
    int value;
  };

  std::array values{Item{1}, Item{2}, Item{3}, Item{4}};
  auto even = [](int value) { return value % 2 == 0; };

  assert(std::ranges::all_of(policy, values.begin(), values.end(), [](int value) { return value > 0; }, &Item::value));
  assert(std::ranges::any_of(policy, values.begin(), values.end(), even, &Item::value));
  assert(std::ranges::none_of(policy, values.begin(), values.end(), [](int value) { return value < 0; }, &Item::value));

  assert(std::ranges::all_of(policy, values, [](int value) { return value < 5; }, &Item::value));
  assert(std::ranges::any_of(policy, values, [](int value) { return value == 3; }, &Item::value));
  assert(std::ranges::none_of(policy, values, [](int value) { return value == 0; }, &Item::value));

  std::array<Item, 0> empty{};
  assert(std::ranges::all_of(policy, empty, even, &Item::value));
  assert(!std::ranges::any_of(policy, empty, even, &Item::value));
  assert(std::ranges::none_of(policy, empty, even, &Item::value));
}

template <class Policy>
concept HasParallelAnyOf = requires(Policy policy, std::forward_list<int>& list) {
  std::ranges::any_of(policy, list, [](int) { return true; });
};

static_assert(!HasParallelAnyOf<decltype(std::execution::seq)>);

int main(int, char**) {
  test(std::execution::seq);
  test(std::execution::par);
  test(std::execution::par_unseq);
  test(std::execution::unseq);
  return 0;
}
