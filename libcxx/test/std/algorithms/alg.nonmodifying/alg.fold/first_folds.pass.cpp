//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// <algorithm>

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

// template<input_iterator I, sentinel_for<I> S,
//          indirectly-binary-left-foldable<iter_value_t<I>, I> F>
//   constexpr see below ranges::fold_left_first_with_iter(I first, S last, F f);
//
// template<input_range R, indirectly-binary-left-foldable<range_value_t<R>, iterator_t<R>> F>
//   constexpr see below ranges::fold_left_first_with_iter(R&& r, F f);
//
// template<input_iterator I, sentinel_for<I> S,
//          indirectly-binary-left-foldable<iter_value_t<I>, I> F>
//   constexpr auto ranges::fold_left_first(I first, S last, F f);
//
// template<input_range R, indirectly-binary-left-foldable<range_value_t<R>, iterator_t<R>> F>
//   constexpr auto ranges::fold_left_first(R&& r, F f);

#include <algorithm>
#include <cassert>
#include <concepts>
#include <forward_list>
#include <functional>
#include <list>
#include <optional>
#include <ranges>
#include <string>
#include <vector>

#include "test_macros.h"

using std::ranges::fold_left_first;
using std::ranges::fold_left_first_with_iter;

template <class Result, class Range, class T>
concept is_first_in_value_result = std::same_as<
    Result,
    std::ranges::fold_left_first_with_iter_result<std::ranges::iterator_t<Range>, std::optional<T>>>;

constexpr bool test_empty_range() {
  auto data = std::vector<int>{};

  {
    is_first_in_value_result<decltype(data), int> decltype(auto) result =
        fold_left_first_with_iter(data.begin(), data.end(), std::plus());
    assert(result.in == data.end());
    assert(!result.value.has_value());
  }

  {
    is_first_in_value_result<decltype(data), int> decltype(auto) result =
        fold_left_first_with_iter(data, std::plus());
    assert(result.in == data.end());
    assert(!result.value.has_value());
  }

  {
    std::same_as<std::optional<int>> decltype(auto) result = fold_left_first(data.begin(), data.end(), std::plus());
    assert(!result.has_value());
  }

  {
    std::same_as<std::optional<int>> decltype(auto) result = fold_left_first(data, std::plus());
    assert(!result.has_value());
  }

  return true;
}

constexpr bool test_single_element() {
  auto data = std::vector<int>{42};

  auto result = fold_left_first(data, std::plus());
  assert(result.has_value());
  assert(*result == 42);

  return true;
}

constexpr bool test_common_range() {
  auto data = std::vector<int>{1, 2, 3, 4, 5};

  // sum: no explicit init, first element seeds the accumulator.
  {
    auto result = fold_left_first(data, std::plus());
    assert(result.has_value());
    assert(*result == 15);
  }

  // order sensitivity: left-fold applies f(acc, elem) left-to-right.
  {
    auto concat = [](std::string acc, std::string const& x) { return acc + x; };
    auto strs   = std::vector<std::string>{"1", "2", "3", "4"};
    auto result = fold_left_first(strs, concat);
    assert(result.has_value());
    assert(*result == "1234");
  }

  return true;
}

void test_non_common_and_input_ranges() {
  // forward_list is a genuine forward (non-sized, non-common in general usage) range.
  {
    auto data   = std::forward_list<int>{2, 4, 6, 8};
    auto result = fold_left_first(data, std::plus());
    assert(result.has_value());
    assert(*result == 20);
  }

  // list, plus a transform view to exercise a non-trivial iterator.
  {
    auto data   = std::list<int>{1, 2, 3};
    auto view   = data | std::views::transform([](int x) { return x * 10; });
    auto result = fold_left_first(view, std::plus());
    assert(result.has_value());
    assert(*result == 60);
  }
}

int main(int, char**) {
  test_empty_range();
  static_assert(test_empty_range());

  test_single_element();
  static_assert(test_single_element());

  test_common_range();
  static_assert(test_common_range());

  test_non_common_and_input_ranges();

  return 0;
}
