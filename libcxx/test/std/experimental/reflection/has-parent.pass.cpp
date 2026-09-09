//===----------------------------------------------------------------------===//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection
#include <meta>

struct S { int member; };
namespace N { struct T {}; }

static_assert(std::meta::has_parent(^^S));
static_assert(std::meta::has_parent(^^S::member));
static_assert(std::meta::has_parent(^^N));
static_assert(!std::meta::has_parent(^^int));
static_assert(!std::meta::has_parent(^^::));
static_assert(!std::meta::has_parent(std::meta::reflect_constant(42)));

int main() {}
