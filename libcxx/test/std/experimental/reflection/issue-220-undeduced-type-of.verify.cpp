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

// <experimental/reflection>
//
// [reflection]
//
// Upstream bloomberg/clang-p2996 issue #220: type_of() on a function whose
// return type is an undeduced 'auto' placeholder must be ill-formed
// ([meta.reflection.queries]p2's has-type condition), not hang. Letting the
// undeduced type through caused the printer's function-type rendering to
// recursively re-query the same placeholder via return_type_of, recursing
// indefinitely instead of producing the required failure.

#include <meta>

struct Undeducible {
  auto operator()();
};

struct Deducible {
  auto operator()() { return 1; }
};

int ordinary(int, int);

consteval bool bad() {
  (void)std::meta::type_of(^^Undeducible::operator());
  // expected-note@-1 {{cannot form a reflection of function 'operator()' whose type 'auto ()' contains an undeduced placeholder}}
  return true;
}

static_assert(bad());
// expected-error@-1 {{static assertion expression is not an integral constant expression}}
// expected-note@-2 {{in call to 'bad()'}}

static_assert(
    std::meta::display_string_of(std::meta::type_of(^^Deducible::operator()))
        .size() > 0);
static_assert(
    std::meta::display_string_of(std::meta::type_of(^^ordinary)).size() > 0);

int main() {
  return 0;
}
