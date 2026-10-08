//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17

#include <ranges>
#include <type_traits>
#include <utility>

struct Bound {
  int value;
  friend constexpr bool operator==(int x, Bound y) { return x == y.value; }
};
using View = std::ranges::iota_view<int, Bound>;
using Sent = std::ranges::sentinel_t<View>;
static_assert(!std::is_constructible_v<Sent, Bound>);
static_assert(std::is_default_constructible_v<Sent>);
static_assert(std::is_same_v<decltype(std::declval<View&>().end()), Sent>);

int main(int, char**) { return 0; }
