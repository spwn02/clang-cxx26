//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

// <algorithm>

// P2440R1: ranges::shift_right
//
// template<permutable I, sentinel_for<I> S>
//   constexpr subrange<I> ranges::shift_right(I first, S last, iter_difference_t<I> n);
// template<forward_range R>
//   requires permutable<iterator_t<R>>
//   constexpr borrowed_subrange_t<R> ranges::shift_right(R&& r, range_difference_t<R> n);

#include <algorithm>
#include <cassert>
#include <list>
#include <ranges>
#include <vector>

#include "test_iterators.h"

int main(int, char**) {
  // (range, n) overload -- basic case.
  {
    std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8};
    auto result          = std::ranges::shift_right(v, 3);
    std::vector<int> want = {1, 2, 3, 4, 5};
    assert(std::ranges::equal(result, want));
    assert(std::ranges::equal(std::ranges::subrange(v.begin() + 3, v.end()), want));
  }

  // (iterator, sentinel, n) overload.
  {
    std::vector<int> v = {1, 2, 3, 4, 5};
    auto result          = std::ranges::shift_right(v.begin(), v.end(), 2);
    std::vector<int> want = {1, 2, 3};
    assert(std::ranges::equal(result, want));
  }

  // n == 0: result is the whole (unchanged) range.
  {
    std::vector<int> v = {1, 2, 3};
    auto result          = std::ranges::shift_right(v, 0);
    assert(std::ranges::equal(result, v));
  }

  // n >= size(): result is empty.
  {
    std::vector<int> v = {1, 2, 3};
    auto result          = std::ranges::shift_right(v, 10);
    assert(result.begin() == result.end());
  }

  // Bidirectional-only range (shift_right's middle implementation strategy).
  {
    std::list<int> l      = {1, 2, 3, 4, 5};
    auto result             = std::ranges::shift_right(l, 2);
    std::vector<int> want   = {1, 2, 3};
    assert(std::ranges::equal(result, want));
  }

  // A sentinel type distinct from the iterator type.
  {
    int a[]             = {1, 2, 3, 4, 5};
    using It             = forward_iterator<int*>;
    using St             = sentinel_wrapper<It>;
    auto result           = std::ranges::shift_right(It(a), St(It(a + 5)), 2);
    std::vector<int> want = {1, 2, 3};
    assert(std::ranges::equal(std::ranges::subrange(result.begin(), result.end()), want));
  }

  return 0;
}
