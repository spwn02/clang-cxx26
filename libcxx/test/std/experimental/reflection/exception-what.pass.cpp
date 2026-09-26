//===----------------------------------------------------------------------===//
//
// Copyright 2026
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

// <meta>
//
// A failing metafunction throws std::meta::exception, and what() carries the specific reason
// (spwn02/clang-cxx26#126).

#include <cassert>
#include <meta>
#include <string_view>

consteval bool reason_is(std::string_view expected, auto&& failing_call) {
  try {
    failing_call();
  } catch (const std::meta::exception& e) {
    return std::string_view(e.what()) == expected;
  }
  return false;
}

static_assert(reason_is("a null reflection has no type", [] { (void)std::meta::type_of(std::meta::info{}); }));
static_assert(reason_is("a null reflection has no parent", [] { (void)std::meta::parent_of(std::meta::info{}); }));
static_assert(reason_is("cannot query the object of a null reflection",
                        [] { (void)std::meta::object_of(std::meta::info{}); }));

int main(int, char**) { return 0; }
