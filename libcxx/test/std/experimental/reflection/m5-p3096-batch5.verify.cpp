//===----------------------------------------------------------------------===//
//
// Copyright 2026 Bloomberg Finance L.P.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20
// ADDITIONAL_COMPILE_FLAGS: -freflection
// ADDITIONAL_COMPILE_FLAGS: -fparameter-reflection

#include <meta>

struct S { int member; };
void fn(int parameter);

// 3096-01, 3096-02: the query requires a function reflection.
static_assert(std::meta::parameters_of(^^S).size() == 0);
// expected-error@-1 {{static assertion expression is not an integral constant expression}}

static_assert(std::meta::return_type_of(^^S) == ^^void);
// expected-error@-1 {{static assertion expression is not an integral constant expression}}

// 3096-03: only a parameter reflection has a variable to recover.
static_assert(std::meta::variable_of(^^S) == std::meta::info{});
// expected-error@-1 {{static assertion expression is not an integral constant expression}}

// 3096-04 and 3096-05 are total functions (P3096R12): invalid domains return
// false rather than producing a diagnostic.
static_assert(!std::meta::has_ellipsis_parameter(^^S));
static_assert(!std::meta::has_default_argument(^^S));

// 3096-06: ordinary declaration queries remain valid for non-parameter
// declarations; parameter-specific cases are covered by the P3096 suite.
static_assert(std::meta::identifier_of(^^S) == "S");

constexpr auto bad_type = std::meta::type_of(^^S::member);
static_assert(bad_type == ^^int);

// 3096-07 is exercised by the front-end requires-expression restriction in
// the existing parameter-reflection tests; retain a direct valid parameter
// query here as a control.
static_assert(std::meta::parameters_of(^^fn).size() == 1);

int main() {}
