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

namespace p2996_batch8 {

constexpr auto bad_members = std::meta::members_of(
    ^^int, std::meta::access_context::current());
// expected-error@-2 {{must be initialized by a constant expression}}
constexpr auto bad_bases = std::meta::bases_of(
    ^^int, std::meta::access_context::current());
// expected-error@-2 {{must be initialized by a constant expression}}
constexpr auto bad_static_members = std::meta::static_data_members_of(
    ^^int, std::meta::access_context::current());
// expected-error@-2 {{must be initialized by a constant expression}}
constexpr auto bad_nonstatic_members = std::meta::nonstatic_data_members_of(
    ^^int, std::meta::access_context::current());
// expected-error@-2 {{must be initialized by a constant expression}}
constexpr auto bad_enumerators = std::meta::enumerators_of(^^int);
// expected-error@-1 {{must be initialized by a constant expression}}

} // namespace p2996_batch8
