// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

#include <meta>
using namespace std::meta;

struct alignas(32) Base { int x; };
struct Empty {};
struct Derived : Base, Empty {};
constexpr auto base = bases_of(^^Derived, access_context::unchecked())[0];
constexpr auto empty = bases_of(^^Derived, access_context::unchecked())[1];
static_assert(size_of(base) == sizeof(Base));
static_assert(alignment_of(base) == alignof(Base));
static_assert(bit_size_of(base) == CHAR_BIT * sizeof(Base));
static_assert(size_of(empty) == sizeof(Empty));
static_assert(alignment_of(empty) == alignof(Empty));
static_assert(bit_size_of(empty) == CHAR_BIT * sizeof(Empty));

int main() {}
