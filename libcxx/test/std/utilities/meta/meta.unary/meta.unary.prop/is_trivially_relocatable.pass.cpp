//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

#include <type_traits>

struct TriviallyRelocatable {
  int value;
};

struct NonTriviallyRelocatable {
  NonTriviallyRelocatable(NonTriviallyRelocatable&&) {}
  ~NonTriviallyRelocatable() {}
};

static_assert(std::is_trivially_relocatable<int>::value);
static_assert(std::is_trivially_relocatable_v<int>);
static_assert(std::is_trivially_relocatable<TriviallyRelocatable>::value);
static_assert(std::is_trivially_relocatable_v<TriviallyRelocatable>);
static_assert(!std::is_trivially_relocatable<NonTriviallyRelocatable>::value);
static_assert(!std::is_trivially_relocatable_v<NonTriviallyRelocatable>);

int main() {}
