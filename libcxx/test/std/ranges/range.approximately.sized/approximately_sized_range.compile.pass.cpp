//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// template<class T>
// concept approximately_sized_range = range<T> && requires(T& t) { ranges::reserve_hint(t); };

#include <ranges>

#include <vector>
#include <forward_list>
#include "test_iterators.h"

// A sized_range satisfies approximately_sized_range through the `size()` branch of reserve_hint.
static_assert(std::ranges::approximately_sized_range<std::vector<int>>);

// A range with neither `size()` nor `reserve_hint()` does not.
static_assert(std::ranges::sized_range<std::forward_list<int>> == false);
static_assert(std::ranges::approximately_sized_range<std::forward_list<int>> == false);

// `forward_iterator` has no `operator-`, so a range using it as both iterator and sentinel is
// not `sized_range` via `sized_sentinel_for` -- unlike a raw pointer, which would be.

// A non-sized range exposing only `reserve_hint()` satisfies the concept.
struct ReserveHintOnlyRange {
  forward_iterator<int*> begin() const;
  forward_iterator<int*> end() const;
  std::size_t reserve_hint() const;
};
static_assert(std::ranges::range<ReserveHintOnlyRange>);
static_assert(!std::ranges::sized_range<ReserveHintOnlyRange>);
static_assert(std::ranges::approximately_sized_range<ReserveHintOnlyRange>);

// A range with neither is not approximately_sized_range, even though it's a range.
struct PlainRange {
  forward_iterator<int*> begin() const;
  forward_iterator<int*> end() const;
};
static_assert(std::ranges::range<PlainRange>);
static_assert(!std::ranges::sized_range<PlainRange>);
static_assert(!std::ranges::approximately_sized_range<PlainRange>);

// Not a range at all.
struct NotARange {};
static_assert(!std::ranges::approximately_sized_range<NotARange>);

int main(int, char**) { return 0; }
