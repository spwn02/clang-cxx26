//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <variant>

#include <type_traits>
#include <variant>

struct MoveConstructibleNotSwappable {
  MoveConstructibleNotSwappable()                                           = default;
  MoveConstructibleNotSwappable(MoveConstructibleNotSwappable&&)            = default;
  MoveConstructibleNotSwappable& operator=(MoveConstructibleNotSwappable&&) = delete;
};

void swap(MoveConstructibleNotSwappable&, MoveConstructibleNotSwappable&) = delete;

static_assert(std::is_move_constructible_v<MoveConstructibleNotSwappable>);
static_assert(!std::is_swappable_v<MoveConstructibleNotSwappable>);
static_assert(requires(std::variant<MoveConstructibleNotSwappable>& value) { value.swap(value); });

int main(int, char**) { return 0; }
