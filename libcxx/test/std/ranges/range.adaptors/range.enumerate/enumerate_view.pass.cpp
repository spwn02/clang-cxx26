//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// <ranges>
// P2164R9 views::enumerate

#include <cassert>
#include <ranges>
#include <tuple>
#include <type_traits>

constexpr bool test() {
  int values[] = {10, 20, 30};

  auto view = std::views::enumerate(values);
  static_assert(std::same_as<decltype(view), std::ranges::enumerate_view<std::ranges::ref_view<int[3]>>>);
  static_assert(std::ranges::common_range<decltype(view)>);
  static_assert(std::ranges::sized_range<decltype(view)>);
  static_assert(std::ranges::enable_borrowed_range<decltype(view)>);

  auto it = view.begin();
  static_assert(std::same_as<decltype(*it), std::tuple<std::ranges::range_difference_t<decltype(view)>, int&>>);
  assert(it.index() == 0);
  auto [index0, value0] = *it;
  assert(index0 == 0 && value0 == 10);

  ++it;
  assert(it.index() == 1);
  auto [index1, value1] = *it;
  assert(index1 == 1 && value1 == 20);

  auto random = view.begin() + 2;
  assert(random.index() == 2 && std::get<1>(*random) == 30);
  assert(view.end() - view.begin() == 3);

  auto empty = std::views::enumerate(std::views::empty<int>);
  assert(empty.begin() == empty.end());

  const auto const_view = std::views::enumerate(std::views::all(values));
  auto const_it = std::as_const(const_view).begin();
  assert(const_it.index() == 0);
  assert(std::get<1>(*const_it) == 10);

  return true;
}

static_assert(test());

int main() { return test() ? 0 : 1; }
