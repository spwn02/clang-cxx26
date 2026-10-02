// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

#include <meta>
using namespace std::meta;

constexpr member_offset offset{-2, 3};
static_assert(offset.total_bits() == -2 * CHAR_BIT + 3);
static_assert(std::is_same_v<decltype(offset.total_bits()), std::ptrdiff_t>);

int main() {}
