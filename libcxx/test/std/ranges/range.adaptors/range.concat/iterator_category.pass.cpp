//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <ranges>

// [range.concat.iterator]
// concat_view::iterator::iterator_category is random_access_iterator_tag only if
// (derived_from<Cs, random_access_iterator_tag> && ...) && concat-is-random-access<Const, Views...>,
// bidirectional_iterator_tag only if the same holds for bidirectional and concat-is-bidirectional.
// A non-common random access view makes the concatenation neither, so the category is not
// stronger than iterator_concept.

#include <cassert>
#include <concepts>
#include <iterator>
#include <ranges>

constexpr bool test() {
  int a[2]{1, 2};
  int b[2]{3, 4};

  {
    // A non-common random access view that is not the last view: concat-is-random-access fails
    // (the first view is not common, so it cannot be traversed from the back).
    auto r1 = std::ranges::subrange(std::counted_iterator(a, 2), std::default_sentinel);
    auto r2 = std::ranges::subrange(b, b + 2);
    static_assert(std::ranges::random_access_range<decltype(r1)>);
    static_assert(!std::ranges::common_range<decltype(r1)>);
    auto c  = std::views::concat(r1, r2);
    using I = decltype(c.begin());
    static_assert(std::same_as<typename I::iterator_concept, std::forward_iterator_tag>);
    static_assert(std::same_as<typename I::iterator_category, std::forward_iterator_tag>);
  }

  {
    // Both common random access: the category is random access.
    auto r1 = std::ranges::subrange(a, a + 2);
    auto r2 = std::ranges::subrange(b, b + 2);
    auto c  = std::views::concat(r1, r2);
    using I = decltype(c.begin());
    static_assert(std::same_as<typename I::iterator_concept, std::random_access_iterator_tag>);
    static_assert(std::same_as<typename I::iterator_category, std::random_access_iterator_tag>);
  }

  return true;
}

int main(int, char**) {
  test();
  static_assert(test());
  return 0;
}
