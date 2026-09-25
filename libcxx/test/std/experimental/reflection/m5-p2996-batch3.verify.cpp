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

// FIXME(spwn02/clang-cxx26#126): the failure reason is no longer part of the
// diagnostic, only the generic "exception thrown here was not caught" note,
// so that is what the expectations below check.

#include <meta>

namespace p2996_batch3 {

// A null reflection has no associated type.
constexpr auto no_type = std::meta::type_of(std::meta::info{});
// expected-error@-1 {{constexpr variable 'no_type' must be initialized by a constant expression}}
// expected-note@* {{exception thrown here was not caught within the constant expression}}

// A null reflection has no containing class or namespace.
constexpr auto no_parent = std::meta::parent_of(std::meta::info{});
// expected-error@-1 {{constexpr variable 'no_parent' must be initialized by a constant expression}}
// expected-note@* {{exception thrown here was not caught within the constant expression}}

// A null reflection does not designate an object.
constexpr auto no_object = std::meta::object_of(std::meta::info{});
// expected-error@-1 {{constexpr variable 'no_object' must be initialized by a constant expression}}
// expected-note@* {{exception thrown here was not caught within the constant expression}}

// A null reflection does not represent a constant value.
constexpr auto no_constant = std::meta::constant_of(std::meta::info{});
// expected-error@-1 {{constexpr variable 'no_constant' must be initialized by a constant expression}}
// expected-note@* {{exception thrown here was not caught within the constant expression}}

// template_of requires a reflection with template arguments.
constexpr auto no_template = std::meta::template_of(std::meta::info{});
// expected-error@-1 {{constexpr variable 'no_template' must be initialized by a constant expression}}

} // namespace p2996_batch3
