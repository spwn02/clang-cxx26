//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// [optional.specalg]: the specialized swap participates for reference values
// even when the referenced type cannot be swapped.

#include <optional>
#include <utility>
#include <type_traits>
static_assert(!std::is_swappable_v<const int&>);
template <class T>
concept HasSpecializedSwap = requires(std::optional<T>& a, std::optional<T>& b) { std::swap<T>(a, b); };
static_assert(HasSpecializedSwap<const int&>);
constexpr bool test_swap() {
  int a = 1;
  int b = 2;
  std::optional<const int&> x(a), y(b);
  std::swap<const int&>(x, y);
  return &*x == &b && &*y == &a;
}
static_assert(test_swap());
