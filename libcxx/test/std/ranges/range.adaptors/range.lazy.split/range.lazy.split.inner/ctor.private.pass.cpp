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

using View = std::ranges::lazy_split_view<std::ranges::subrange<int*>, std::ranges::single_view<int>>;
using Outer = std::ranges::iterator_t<View>;
using Inner = decltype(std::declval<std::ranges::range_value_t<View>&>().begin());
static_assert(!std::is_constructible_v<Inner, Outer>);

int main(int, char**) { return 0; }
