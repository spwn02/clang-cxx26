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

constexpr bool test() {
  int value = 42;
  std::optional<int&> input(value);

  static_assert(std::same_as<decltype(std::views::as_const(input)), std::optional<const int&>>);
  std::same_as<std::optional<const int&>> decltype(auto) result = std::views::as_const(input);
  static_assert(std::same_as<decltype(*result), const int&>);
  assert(result && *result == 42);

  return true;
}

int main(int, char**) {
  test();
  static_assert(test());
  return 0;
}
