//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// <algorithm>

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

// template<bidirectional_iterator I, sentinel_for<I> S, class T,
//          indirectly-binary-right-foldable<T, I> F>
//   constexpr auto ranges::fold_right(I first, S last, T init, F f);
//
// template<bidirectional_range R, class T, indirectly-binary-right-foldable<T, iterator_t<R>> F>
//   constexpr auto ranges::fold_right(R&& r, T init, F f);
//
// template<bidirectional_iterator I, sentinel_for<I> S,
//          indirectly-binary-right-foldable<iter_value_t<I>, I> F>
//   constexpr auto ranges::fold_right_last(I first, S last, F f);
//
// template<bidirectional_range R, indirectly-binary-right-foldable<range_value_t<R>, iterator_t<R>> F>
//   constexpr auto ranges::fold_right_last(R&& r, F f);

#include <algorithm>
#include <cassert>
#include <concepts>
#include <deque>
#include <functional>
#include <list>
#include <optional>
#include <ranges>
#include <string>
#include <vector>

#include "test_macros.h"

using std::ranges::fold_right;
using std::ranges::fold_right_last;

constexpr bool test_empty_range() {
  auto data = std::vector<int>{};

  {
    std::same_as<int> decltype(auto) result = fold_right(data.begin(), data.end(), 7, std::plus());
    assert(result == 7);
  }

  {
    std::same_as<int> decltype(auto) result = fold_right(data, 7, std::plus());
    assert(result == 7);
  }

  {
    std::same_as<std::optional<int>> decltype(auto) result = fold_right_last(data.begin(), data.end(), std::plus());
    assert(!result.has_value());
  }

  {
    std::same_as<std::optional<int>> decltype(auto) result = fold_right_last(data, std::plus());
    assert(!result.has_value());
  }

  return true;
}

constexpr bool test_associativity_order() {
  // fold_right computes f(x1, f(x2, ..., f(xn, init))): right-to-left, elementwise
  // first argument, accumulator second. Subtraction makes the order observable.
  auto data = std::vector<int>{1, 2, 3, 4};

  // 1 - (2 - (3 - (4 - 0))) = 1 - (2 - (3 - 4)) = 1 - (2 - (-1)) = 1 - 3 = -2
  {
    auto result = fold_right(data, 0, std::minus());
    assert(result == -2);
  }

  // fold_right_last seeds init with the *last* element and folds the rest:
  // 1 - (2 - (3 - 4)) = 1 - (2 - (-1)) = 1 - 3 = -2
  {
    auto result = fold_right_last(data, std::minus());
    assert(result.has_value());
    assert(*result == -2);
  }

  // string concatenation makes right-to-left visitation directly observable.
  {
    auto strs   = std::vector<std::string>{"a", "b", "c"};
    auto concat = [](std::string const& x, std::string acc) { return x + acc; };
    auto result = fold_right(strs, std::string("!"), concat);
    assert(result == "abc!");
  }

  return true;
}

constexpr bool test_sum_agrees_with_left() {
  // For an associative/commutative operator, left and right folds agree.
  auto data = std::vector<int>{1, 2, 3, 4, 5};
  assert(fold_right(data, 0, std::plus()) == std::ranges::fold_left(data, 0, std::plus()));
  assert(*fold_right_last(data, std::plus()) == *std::ranges::fold_left_first(data, std::plus()));
  return true;
}

void test_bidirectional_containers() {
  {
    auto data   = std::list<int>{10, 20, 30};
    auto result = fold_right(data, 0, std::plus());
    assert(result == 60);
    auto last = fold_right_last(data, std::plus());
    assert(last.has_value() && *last == 60);
  }

  {
    auto data   = std::deque<int>{1, 1, 1, 1};
    auto result = fold_right(data, 0, std::plus());
    assert(result == 4);
  }

  // iterator/sentinel overload directly.
  {
    auto data   = std::vector<int>{5, 6, 7};
    auto result = fold_right(data.begin(), data.end(), 0, std::plus());
    assert(result == 18);
    auto last = fold_right_last(data.begin(), data.end(), std::plus());
    assert(last.has_value() && *last == 18);
  }
}

int main(int, char**) {
  test_empty_range();
  static_assert(test_empty_range());

  test_associativity_order();
  static_assert(test_associativity_order());

  test_sum_agrees_with_left();
  static_assert(test_sum_agrees_with_left());

  test_bidirectional_containers();

  return 0;
}
