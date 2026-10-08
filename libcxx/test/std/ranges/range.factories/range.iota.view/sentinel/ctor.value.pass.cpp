//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17

// constexpr explicit sentinel(Bound bound);

#include <cassert>
#include <ranges>
#include <type_traits>

#include "test_macros.h"
#include "../types.h"

constexpr bool test() {
  {
    using Sent = std::ranges::sentinel_t<std::ranges::iota_view<int, IntSentinelWith<int>>>;
    using Iter = std::ranges::iterator_t<std::ranges::iota_view<int, IntSentinelWith<int>>>;
    auto view = std::ranges::iota_view<int, IntSentinelWith<int>>{42, IntSentinelWith<int>(42)};
    auto sent = view.end();
    assert(sent == view.begin());
  }
  {
    using Sent = std::ranges::sentinel_t<std::ranges::iota_view<SomeInt, IntSentinelWith<SomeInt>>>;
    using Iter = std::ranges::iterator_t<std::ranges::iota_view<SomeInt, IntSentinelWith<SomeInt>>>;
    auto view = std::ranges::iota_view<SomeInt, IntSentinelWith<SomeInt>>{SomeInt(42), IntSentinelWith<SomeInt>(SomeInt(42))};
    auto sent = view.end();
    assert(sent == view.begin());
  }
  {
    using Sent = std::ranges::sentinel_t<std::ranges::iota_view<SomeInt, IntSentinelWith<SomeInt>>>;
    static_assert(!std::is_convertible_v<Sent, IntSentinelWith<SomeInt>>);
    static_assert(!std::is_constructible_v<Sent, IntSentinelWith<SomeInt>>);
  }

  return true;
}

int main(int, char**) {
  test();
  static_assert(test());

  return 0;
}
