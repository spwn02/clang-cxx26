//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: no-ranges

// [ranges.syn]: const_iterator_t is decltype(ranges::cbegin(declval<R&>())), const_sentinel_t is
// decltype(ranges::cend(declval<R&>())) and range_const_reference_t only needs a range;
// [range.sized]: sized_range refines approximately_sized_range.

#include <concepts>
#include <iterator>
#include <ranges>
#include <vector>

namespace r = std::ranges;

static_assert(std::same_as<r::const_iterator_t<std::vector<int>>, decltype(r::cbegin(std::declval<std::vector<int>&>()))>);
static_assert(std::same_as<r::const_sentinel_t<std::vector<int>>, decltype(r::cend(std::declval<std::vector<int>&>()))>);
static_assert(std::same_as<r::const_iterator_t<const std::vector<int>>,
                           decltype(r::cbegin(std::declval<const std::vector<int>&>()))>);

// range_const_reference_t of a range whose iterator is not an input iterator.
struct output_only_iterator {
  using iterator_concept  = std::output_iterator_tag;
  using value_type        = int;
  using difference_type   = std::ptrdiff_t;
  int* p;
  int& operator*() const { return *p; }
  output_only_iterator& operator++() {
    ++p;
    return *this;
  }
  void operator++(int) { ++p; }
  friend bool operator==(output_only_iterator a, output_only_iterator b) { return a.p == b.p; }
};
struct output_only_range {
  output_only_iterator begin();
  output_only_iterator end();
};
static_assert(r::range<output_only_range> && !r::input_range<output_only_range>);
static_assert(std::same_as<r::range_const_reference_t<output_only_range>, const int&>);

template <r::approximately_sized_range T>
constexpr int f(T&) {
  return 1;
}
template <r::sized_range T>
constexpr int f(T&) {
  return 2;
}
constexpr int a[] = {1, 2};
static_assert(f(a) == 2); // a more constrained overload: sized_range subsumes approximately_sized_range

int main(int, char**) { return 0; }
