//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

// <ranges>

// LWG 3761 / P2374R4: cartesian_product_view::iterator
//   friend constexpr difference_type operator-(const iterator&, default_sentinel_t)
//     requires cartesian-is-sized-sentinel<...>;
//   friend constexpr difference_type operator-(default_sentinel_t, const iterator&)
//     requires cartesian-is-sized-sentinel<...>;

#include <array>
#include <cassert>
#include <iterator>
#include <ranges>
#include <vector>

template <class It>
concept can_subtract_sentinel = requires(It it) {
  it - std::default_sentinel;
  std::default_sentinel - it;
};

constexpr bool test() {
  std::array<int, 3> a{1, 2, 3};
  std::array<int, 2> b{4, 5};
  {
    auto view = std::views::cartesian_product(a, b);
    using It  = decltype(view.begin());
    static_assert(can_subtract_sentinel<It>);
    static_assert(std::same_as<decltype(view.begin() - std::default_sentinel), std::ranges::range_difference_t<decltype(view)>>);
    auto it = view.begin();
    for (int i = 0; i < 6; ++i, ++it) {
      assert(it - std::default_sentinel == -(6 - i));
      assert(std::default_sentinel - it == 6 - i);
    }
    assert(it == std::default_sentinel);
    assert(it - std::default_sentinel == 0);
  }
  {
    // A first range that is not a common range still works: its sentinel is a sized sentinel.
    auto first = std::views::iota(0, 4) | std::views::take_while([](int) { return true; });
    (void)first;
    auto view = std::views::cartesian_product(std::views::iota(0, 4), b);
    auto it   = view.begin();
    assert(std::default_sentinel - it == 8);
    ++it;
    assert(std::default_sentinel - it == 7);
  }
  {
    // const iterator
    const auto view = std::views::cartesian_product(a, b);
    auto it         = view.begin();
    ++it;
    assert(std::default_sentinel - it == 5);
    assert(it - std::default_sentinel == -5);
  }
  {
    // An empty range: begin() is already at the end.
    std::array<int, 0> empty{};
    auto view = std::views::cartesian_product(a, empty);
    assert(view.begin() - std::default_sentinel == 0);
  }
  {
    // Unsized ranges do not get the operators.
    auto unsized = std::views::cartesian_product(std::views::iota(0));
    static_assert(!can_subtract_sentinel<decltype(unsized.begin())>);
    auto inner_unsized = std::views::cartesian_product(a, std::views::iota(0, 3) | std::views::filter([](int) { return true; }));
    static_assert(!can_subtract_sentinel<decltype(inner_unsized.begin())>);
  }
  return true;
}

int main(int, char**) {
  test();
  static_assert(test());
  return 0;
}
