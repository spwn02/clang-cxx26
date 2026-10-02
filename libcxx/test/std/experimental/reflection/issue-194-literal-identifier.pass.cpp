// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

#include <meta>
using namespace std::meta;

int operator""_lit(unsigned long long);
static_assert(has_identifier(^^operator""_lit));
static_assert(identifier_of(^^operator""_lit) == "_lit");
static_assert(u8identifier_of(^^operator""_lit) == u8"_lit");

int main() {}
