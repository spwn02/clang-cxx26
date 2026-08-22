//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// std::ranges::as_input_view, std::views::as_input

#include <ranges>

#include <algorithm>
#include <cassert>
#include <concepts>
#include <vector>
#include "test_iterators.h"
#include "test_macros.h"

// A pure input range (non-common, not a forward_range) -- `views::as_input` should pass this
// through as `views::all(E)` without wrapping in `as_input_view`.
struct PureInputRange {
  int* data_;
  int size_;
  using Iterator = cpp20_input_iterator<int*>;
  constexpr Iterator begin() const { return Iterator(data_); }
  constexpr sentinel_wrapper<Iterator> end() const { return sentinel_wrapper<Iterator>(Iterator(data_ + size_)); }
};
static_assert(std::ranges::input_range<PureInputRange>);
static_assert(!std::ranges::common_range<PureInputRange>);
static_assert(!std::ranges::forward_range<PureInputRange>);

constexpr bool testPassthrough() {
  int data[] = {1, 2, 3};
  PureInputRange range{data, 3};
  std::same_as<std::ranges::ref_view<PureInputRange>> decltype(auto) result = std::views::as_input(range);
  assert(std::ranges::equal(result, data));
  return true;
}

constexpr bool testWrapsForwardRange() {
  // `std::vector` is a forward_range (in fact contiguous/common) -- `as_input` must degrade it
  // to input-only, non-common.
  std::vector<int> v = {1, 2, 3, 4};
  std::same_as<std::ranges::as_input_view<std::ranges::ref_view<std::vector<int>>>> decltype(auto) result =
      std::views::as_input(v);
  static_assert(std::ranges::input_range<decltype(result)>);
  static_assert(!std::ranges::forward_range<decltype(result)>);
  static_assert(!std::ranges::common_range<decltype(result)>);
  assert(std::ranges::equal(result, v));
  return true;
}

// An input-only, common range (self-sentinel input iterator, no `iterator_category` derived
// from `forward_iterator_tag`). Only the "input-only AND non-common" combination is passed
// through unwrapped -- being a common_range on its own must still trigger wrapping.
struct CommonInputIterator {
  using value_type       = int;
  using difference_type  = std::ptrdiff_t;
  using iterator_concept = std::input_iterator_tag;

  int* p_ = nullptr;

  constexpr int operator*() const { return *p_; }
  constexpr CommonInputIterator& operator++() {
    ++p_;
    return *this;
  }
  constexpr void operator++(int) { ++*this; }
  friend constexpr bool operator==(const CommonInputIterator& x, const CommonInputIterator& y) {
    return x.p_ == y.p_;
  }
};
static_assert(std::input_iterator<CommonInputIterator>);
static_assert(std::sentinel_for<CommonInputIterator, CommonInputIterator>);

struct CommonInputRange {
  int* data_;
  int size_;
  constexpr CommonInputIterator begin() const { return CommonInputIterator{data_}; }
  constexpr CommonInputIterator end() const { return CommonInputIterator{data_ + size_}; }
};
static_assert(std::ranges::input_range<CommonInputRange>);
static_assert(std::ranges::common_range<CommonInputRange>);
static_assert(!std::ranges::forward_range<CommonInputRange>);

constexpr bool testWrapsCommonInputRange() {
  int data[] = {5, 6, 7};
  CommonInputRange range{data, 3};
  std::same_as<std::ranges::as_input_view<std::ranges::ref_view<CommonInputRange>>> decltype(auto) result =
      std::views::as_input(range);
  static_assert(!std::ranges::common_range<decltype(result)>);
  assert(std::ranges::equal(result, data));
  return true;
}

constexpr bool testPipeAndSize() {
  std::vector<int> v = {1, 2, 3};
  auto result = v | std::views::as_input;
  assert(std::ranges::equal(result, v));
  assert(result.size() == 3);
  return true;
}

int main(int, char**) {
  testPassthrough();
  static_assert(testPassthrough());

  testWrapsForwardRange();
  static_assert(testWrapsForwardRange());

  testWrapsCommonInputRange();
  static_assert(testWrapsCommonInputRange());

  testPipeAndSize();
  static_assert(testPipeAndSize());

  // Not a borrowed_range, even when the underlying view is.
  static_assert(!std::ranges::enable_borrowed_range<std::ranges::as_input_view<std::ranges::ref_view<std::vector<int>>>>);
  static_assert(!std::ranges::borrowed_range<std::ranges::as_input_view<std::ranges::ref_view<std::vector<int>>>>);

  return 0;
}
