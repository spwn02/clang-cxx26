//===----------------------------------------------------------------------===//
//
// Copyright 2026
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

#include <meta>

namespace p2996_batch11 {

constexpr auto bad_type = std::meta::data_member_spec(std::meta::reflect_constant(1));
// expected-error@-1 {{must be initialized by a constant expression}}
constexpr auto bad_name = std::meta::data_member_spec(^^int, {.name = "1bad"});
// expected-error@-1 {{must be initialized by a constant expression}}
constexpr auto both_width_alignment = std::meta::data_member_spec(
    ^^int, {.alignment = 8, .width = 1});
// expected-error@-2 {{must be initialized by a constant expression}}
constexpr auto negative_width = std::meta::data_member_spec(
    ^^int, {.width = -1});
// expected-error@-2 {{must be initialized by a constant expression}}
constexpr auto bad_alignment = std::meta::data_member_spec(
    ^^int, {.alignment = 3});
// expected-error@-2 {{must be initialized by a constant expression}}
constexpr auto bad_bitfield_type = std::meta::data_member_spec(
    ^^float, {.width = 1});
// expected-error@-2 {{must be initialized by a constant expression}}

} // namespace p2996_batch11
