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

namespace p2996_batch12 {

void requires_probe(int parameter) {
  (void)requires(int local) { ^^local; };
  // expected-error@-1 {{cannot take the reflection of a local parameter of a requires-expression}}
}

struct Base { void f(); };
struct Derived : Base { using Base::f; };
constexpr auto using_declarator = ^^Derived::f;
// expected-error@-1 {{cannot take the reflection of a using-declarator}}

} // namespace p2996_batch12
