//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17

// [range.lazy.split.outer.value] / LWG4013: outer-iterator::value_type has no default constructor, and its
// constructor from an outer-iterator is exposition-only (not callable by programs).

#include <ranges>
#include <type_traits>

#include "../types.h"

static_assert(!std::is_default_constructible_v<ValueTypeForward>);
static_assert(!std::is_default_constructible_v<ValueTypeInput>);
static_assert(!std::is_constructible_v<ValueTypeForward, OuterIterForward>);
static_assert(!std::is_constructible_v<ValueTypeInput, OuterIterInput>);
static_assert(std::is_copy_constructible_v<ValueTypeForward>);
