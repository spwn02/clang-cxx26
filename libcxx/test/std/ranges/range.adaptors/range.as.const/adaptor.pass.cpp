//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

// ADDITIONAL_COMPILE_FLAGS: -fexperimental-library

#include <ranges>

#include <cassert>
#include <concepts>
#include <optional>
#include <span>
#include <vector>

#include "test_macros.h"

constexpr bool test() {
#if TEST_STD_VER >= 26 // optional<T&> and views::as_const(optional) are C++26
  int value = 42;
  std::optional<int&> input(value);

  static_assert(std::same_as<decltype(std::views::as_const(input)), std::optional<const int&>>);
  std::same_as<std::optional<const int&>> decltype(auto) result = std::views::as_const(input);
  static_assert(std::same_as<decltype(*result), const int&>);
  assert(result && *result == 42);
#endif

  int values[] = {1, 2, 3};
  std::span<int> span_input(values);
  static_assert(std::same_as<decltype(std::views::as_const(span_input)), std::span<const int>>);

  std::span<const int> constant_span(values);
  static_assert(std::same_as<decltype(std::views::as_const(constant_span)), std::span<const int>>);

  const std::vector<int> constant_vector(values, values + 3);
  static_assert(std::same_as<decltype(std::views::as_const(constant_vector)),
                             std::ranges::ref_view<const std::vector<int>>>);

  std::ranges::empty_view<int> empty;
  static_assert(std::same_as<decltype(std::views::as_const(empty)), std::ranges::empty_view<const int>>);

  std::vector<int> plain_lvalue(values, values + 3);
  static_assert(std::same_as<decltype(std::views::as_const(plain_lvalue)),
                             std::ranges::ref_view<const std::vector<int>>>);

  std::vector<int> vector_input(values, values + 3);
  std::ranges::ref_view<std::vector<int>> ref(vector_input);
  static_assert(std::same_as<decltype(std::views::as_const(ref)), std::ranges::ref_view<const std::vector<int>>>);

  return true;
}

int main(int, char**) {
  test();
  static_assert(test());
  return 0;
}
