// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

#include <meta>
using namespace std::meta;

struct C { int x; };
enum class E { x };
using CAlias = const C;
using EAlias = const E;
static_assert(is_enumerable_type(^^C) && is_enumerable_type(^^E));
static_assert(is_enumerable_type(^^const C));
static_assert(is_enumerable_type(^^volatile C));
static_assert(is_enumerable_type(^^const volatile E));
static_assert(is_enumerable_type(^^CAlias));
static_assert(is_enumerable_type(^^EAlias));
static_assert(members_of(^^const C, access_context::unchecked()).size() ==
              members_of(^^C, access_context::unchecked()).size());

int main() {}
