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

namespace p2996_batch7 {

// 2996-16: template_arguments_of requires template arguments.
constexpr auto no_template_args = std::meta::template_arguments_of(^^int);
// expected-error@-1 {{must be initialized by a constant expression}}

// 2996-17: the access context's via reflection must designate a class.
constexpr auto bad_access = std::meta::is_accessible(
    ^^int, std::meta::access_context::current().via(^^int));
// expected-error@-2 {{must be initialized by a constant expression}}

// 2996-18: the reflected argument must be a class type.
constexpr auto bad_members = std::meta::has_inaccessible_nonstatic_data_members(
    ^^int, std::meta::access_context::current());
// expected-error@-2 {{must be initialized by a constant expression}}

// 2996-20: the reflected argument must be a class type.
constexpr auto bad_bases = std::meta::has_inaccessible_bases(
    ^^int, std::meta::access_context::current());
// expected-error@-2 {{must be initialized by a constant expression}}

} // namespace p2996_batch7
