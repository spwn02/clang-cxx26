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

// <experimental/reflection>
//
// [reflection]
//
// P3096R12 [meta.reflection.parameter]: has_ellipsis_parameter and
// has_default_argument are total functions over 'info' (no Constant When
// clause -- has_default_argument's was explicitly removed per a LEWG poll
// in R12), so both must return false for any reflection that isn't
// applicable, never diagnose/fail evaluation.

#include <meta>

using namespace std::meta;

struct S {
  int x;
};

int f(int a, ...);
void g(int a = 5);
void h(int a);

static_assert(has_ellipsis_parameter(^^int) == false);
static_assert(has_default_argument(^^int) == false);
static_assert(has_ellipsis_parameter(^^S) == false);
static_assert(has_default_argument(^^S) == false);
static_assert(has_ellipsis_parameter(^^S::x) == false);
static_assert(has_default_argument(^^S::x) == false);

static_assert(has_ellipsis_parameter(^^f) == true);
static_assert(has_default_argument(^^f) == false);

static_assert(has_ellipsis_parameter(^^g) == false);
static_assert(has_default_argument(parameters_of(^^g)[0]) == true);

static_assert(has_ellipsis_parameter(parameters_of(^^h)[0]) == false);
static_assert(has_default_argument(parameters_of(^^h)[0]) == false);

int main() {
  return 0;
}
