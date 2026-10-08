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

#include "test_iterators.h"

using Base = std::ranges::subrange<int*, sentinel_wrapper<int*>>;

using Inner = std::ranges::subrange<int*>;
using Outer = std::ranges::subrange<Inner*, sentinel_wrapper<Inner*>>;
using View = std::ranges::join_view<Outer>;
using Sent = std::ranges::sentinel_t<View>;
static_assert(!std::is_constructible_v<Sent, View&>);
static_assert(std::is_same_v<decltype(std::declval<View&>().end()), Sent>);

int main(int, char**) { return 0; }
