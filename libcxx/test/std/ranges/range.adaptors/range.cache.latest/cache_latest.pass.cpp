//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// std::ranges::cache_latest_view, std::views::cache_latest

#include <ranges>

#include <algorithm>
#include <cassert>
#include <concepts>
#include <vector>
#include "test_iterators.h"
#include "test_macros.h"

// A pure input range whose `operator*` counts how many times it's called at the current
// position -- lets us prove `cache_latest_view` memoizes the current element instead of
// recomputing it on repeated dereference.
struct CountingInputRange {
  int* data_;
  int size_;
  int* deref_count_;

  struct Iterator {
    int* data_        = nullptr;
    int* deref_count_ = nullptr;
    int pos_          = 0;

    using difference_type   = std::ptrdiff_t;
    using value_type        = int;
    using iterator_concept  = std::input_iterator_tag;

    Iterator() = default;
    constexpr int operator*() const {
      ++*deref_count_;
      return data_[pos_];
    }
    constexpr Iterator& operator++() {
      ++pos_;
      return *this;
    }
    constexpr void operator++(int) { ++*this; }
    friend constexpr bool operator==(const Iterator& x, const Iterator& y) { return x.pos_ == y.pos_; }
  };

  constexpr Iterator begin() const {
    Iterator it;
    it.data_        = data_;
    it.deref_count_ = deref_count_;
    it.pos_         = 0;
    return it;
  }
  constexpr Iterator end() const {
    Iterator it;
    it.data_        = data_;
    it.deref_count_ = deref_count_;
    it.pos_         = size_;
    return it;
  }
};
static_assert(std::ranges::input_range<CountingInputRange>);
static_assert(!std::ranges::forward_range<CountingInputRange>);

constexpr bool testCaching() {
  int data[] = {10, 20, 30};
  int deref_count = 0;
  CountingInputRange range{data, 3, &deref_count};

  auto view = range | std::views::cache_latest;
  auto it   = view.begin();

  // Multiple dereferences at the same position must only compute once.
  assert(*it == 10);
  assert(*it == 10);
  assert(*it == 10);
  assert(deref_count == 1);

  // Advancing invalidates the cache and forces a fresh computation.
  ++it;
  assert(deref_count == 1);
  assert(*it == 20);
  assert(deref_count == 2);
  assert(*it == 20);
  assert(deref_count == 2);

  ++it;
  assert(*it == 30);
  assert(deref_count == 3);

  ++it;
  assert(it == view.end());

  return true;
}

constexpr bool testAdaptorAndAccessors() {
  std::vector<int> v = {1, 2, 3, 4};
  {
    std::same_as<std::ranges::cache_latest_view<std::ranges::ref_view<std::vector<int>>>> decltype(auto) result =
        std::views::cache_latest(v);
    assert(std::ranges::equal(result, v));
  }
  {
    // Pipe syntax.
    auto result = v | std::views::cache_latest;
    assert(std::ranges::equal(result, v));
  }
  {
    // size() forwards to the underlying sized range.
    auto result = v | std::views::cache_latest;
    assert(result.size() == 4);
  }
  return true;
}

int main(int, char**) {
  testCaching();
  static_assert(testCaching());

  testAdaptorAndAccessors();
  static_assert(testAdaptorAndAccessors());

  // Not a borrowed_range: the iterator holds a pointer back to the view (for the cache), so
  // it cannot safely outlive the view the way a true borrowed range's iterator can.
  static_assert(!std::ranges::enable_borrowed_range<std::ranges::cache_latest_view<std::ranges::ref_view<std::vector<int>>>>);
  static_assert(!std::ranges::borrowed_range<std::ranges::cache_latest_view<std::ranges::ref_view<std::vector<int>>>>);

  return 0;
}
