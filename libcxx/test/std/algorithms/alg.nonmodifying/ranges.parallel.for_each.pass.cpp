//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: libcpp-has-no-incomplete-pstl

// P3179R9: ranges execution-policy overloads for for_each and for_each_n.

#include <algorithm>
#include <array>
#include <cassert>
#include <execution>
#include <forward_list>
#include <type_traits>

template <class Policy>
void test(Policy&& policy) {
  struct Item {
    int value;

    bool operator==(const Item&) const = default;
  };

  std::array values{Item{1}, Item{2}, Item{3}, Item{4}};
  auto increment = [](int& value) { ++value; };

  static_assert(std::same_as<decltype(std::ranges::for_each(policy, values.begin(), values.end(), increment, &Item::value)),
                             decltype(values.begin())>);
  static_assert(std::same_as<decltype(std::ranges::for_each_n(policy, values.begin(), 2, increment, &Item::value)),
                             decltype(values.begin())>);

  assert(std::ranges::for_each(policy, values.begin(), values.end(), increment, &Item::value) == values.end());
  assert((values == std::array{Item{2}, Item{3}, Item{4}, Item{5}}));

  assert(std::ranges::for_each(policy, values, increment, &Item::value) == values.end());
  assert((values == std::array{Item{3}, Item{4}, Item{5}, Item{6}}));

  assert(std::ranges::for_each_n(policy, values.begin(), 2, increment, &Item::value) == values.begin() + 2);
  assert((values == std::array{Item{4}, Item{5}, Item{5}, Item{6}}));
}

template <class Policy>
concept HasParallelForEach = requires(Policy policy, std::forward_list<int>& list) {
  std::ranges::for_each(policy, list, [](int&) {});
};

template <class Policy>
concept HasParallelForEachN = requires(Policy policy, std::forward_list<int>& list) {
  std::ranges::for_each_n(policy, list.begin(), 1, [](int&) {});
};

static_assert(!HasParallelForEach<decltype(std::execution::seq)>);
static_assert(!HasParallelForEachN<decltype(std::execution::seq)>);

int main(int, char**) {
  test(std::execution::seq);
  test(std::execution::par);
  test(std::execution::par_unseq);
  test(std::execution::unseq);
  return 0;
}
