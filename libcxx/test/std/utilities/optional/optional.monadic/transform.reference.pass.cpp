//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <optional>

// C++26 permits transform to return an optional reference.

#include <cassert>
#include <concepts>
#include <memory>
#include <optional>
#include <utility>

int main(int, char**) {
  int referred = 42;
  std::optional<int> value{1};
  std::optional<int&> reference(value.value());

  auto lvalue = value.transform([&](int&) -> int& { return referred; });
  static_assert(std::same_as<decltype(lvalue), std::optional<int&>>);
  assert(lvalue && std::addressof(*lvalue) == std::addressof(referred));

  const auto& const_value = value;
  auto const_lvalue       = const_value.transform([&](const int&) -> const int& { return referred; });
  static_assert(std::same_as<decltype(const_lvalue), std::optional<const int&>>);
  assert(const_lvalue && std::addressof(*const_lvalue) == std::addressof(referred));

  auto rvalue = std::move(value).transform([&](int&&) -> int& { return referred; });
  static_assert(std::same_as<decltype(rvalue), std::optional<int&>>);
  assert(rvalue && std::addressof(*rvalue) == std::addressof(referred));

  auto const_rvalue = std::move(const_value).transform([&](const int&&) -> const int& { return referred; });
  static_assert(std::same_as<decltype(const_rvalue), std::optional<const int&>>);
  assert(const_rvalue && std::addressof(*const_rvalue) == std::addressof(referred));

  auto reference_result = reference.transform([&](int&) -> int& { return referred; });
  static_assert(std::same_as<decltype(reference_result), std::optional<int&>>);
  assert(reference_result && std::addressof(*reference_result) == std::addressof(referred));

  return 0;
}
