//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

#include <cassert>
#include <iterator>
#include <ranges>

#include "integer_class.h"

using Difference = integer_class::Signed;

struct Iterator {
  using difference_type = Difference;
  using value_type = int;
  int* p = nullptr;

  constexpr int& operator*() const { return *p; }
  constexpr Iterator& operator++() { ++p; return *this; }
  constexpr Iterator operator++(int) { Iterator copy = *this; ++*this; return copy; }
  friend constexpr bool operator==(Iterator, Iterator) = default;
  friend constexpr Difference operator-(Iterator x, Iterator y) { return Difference(x.p - y.p); }
};

static_assert(std::input_or_output_iterator<Iterator>);
static_assert(std::sentinel_for<std::default_sentinel_t, std::counted_iterator<Iterator>>);

int main(int, char**) {
  int values[] = {1, 2, 3, 4};
  using Counted = std::counted_iterator<Iterator>;
  using Common = std::common_iterator<Counted, std::default_sentinel_t>;
  Common first{Counted{Iterator{values}, Difference{4}}};
  Common last{std::default_sentinel};
  assert(std::ranges::distance(first, last) == Difference{4});
  return 0;
}
