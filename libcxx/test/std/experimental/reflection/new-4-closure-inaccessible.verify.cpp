//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

// M5 checklist row 2996-19 (P2996R13): has_inaccessible_nonstatic_data_members
// is ill-formed when r represents a closure type.

#include <meta>

struct Valid {
  int value;
};

constexpr auto closure_type = ^^decltype([] {});
constexpr bool bad = std::meta::has_inaccessible_nonstatic_data_members(
    closure_type, std::meta::access_context::current());
// expected-error@-2 {{must be initialized by a constant expression}}

constexpr bool good = std::meta::has_inaccessible_nonstatic_data_members(
    ^^Valid, std::meta::access_context::current());
static_assert(!good);
