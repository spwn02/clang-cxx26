//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

#include <meta>

using info = std::meta::info;
using Alias = const volatile info;
static_assert(std::meta::is_reflection_type(^^info));
static_assert(std::meta::is_reflection_type(^^const info));
static_assert(std::meta::is_reflection_type(^^volatile info));
static_assert(std::meta::is_reflection_type(^^const volatile info));
static_assert(std::meta::is_reflection_type(^^Alias));
static_assert(!std::meta::is_reflection_type(^^int));
static_assert(!std::meta::is_reflection_type(^^info&));

consteval bool throws(std::meta::info r) {
  try { (void)std::meta::is_reflection_type(r); }
  catch (const std::meta::exception&) { return true; }
  return false;
}
static_assert(throws(^^::));
static_assert(throws(std::meta::reflect_constant(42)));
static_assert(throws(info{}));

int main(int, char**) { return 0; }
