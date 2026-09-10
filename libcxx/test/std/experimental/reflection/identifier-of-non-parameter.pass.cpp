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
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

// M5 checklist row 3096-06: P3096R12 does not restrict identifier_of,
// u8identifier_of, or has_identifier to function-parameter reflections --
// it specifies has_identifier for named entities generally and makes
// identifier_of/u8identifier_of conditional on has_identifier(r); parameter
// reflections merely receive additional naming rules on top. A named type,
// data member, and function must accept these three queries the same way a
// named parameter does. (An earlier M5 session's NEW-8 claim that these
// three were ill-formed for non-parameter reflections misread the paper;
// see the corrected entry in docs/REFLECTION_GAPS.md. type_of(^^S) is a
// separate, still-correctly-rejected case and is out of scope here.)

#include <meta>

using namespace std::meta;

struct S {
  int x;
};

void f(int a, int b);

static_assert(has_identifier(^^S));
static_assert(identifier_of(^^S) == "S");
static_assert(u8identifier_of(^^S) == u8"S");

static_assert(has_identifier(^^S::x));
static_assert(identifier_of(^^S::x) == "x");
static_assert(u8identifier_of(^^S::x) == u8"x");

static_assert(has_identifier(^^f));
static_assert(identifier_of(^^f) == "f");
static_assert(u8identifier_of(^^f) == u8"f");

static_assert(has_identifier(parameters_of(^^f)[0]));
static_assert(identifier_of(parameters_of(^^f)[0]) == "a");

int main() {
  return 0;
}
