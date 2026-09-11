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
// Upstream bloomberg/clang-p2996 issue #237: aliasing a closure (lambda)
// type through a dependent splice ('using ct = typename[:cr:];', where 'cr'
// reflects the type of an 'auto'-declared closure variable) was rejected
// outright with "'auto' not allowed in type alias". Root cause:
// 'decltype(e)' for an id-expression 'e' naming an 'auto'-declared entity
// yields the entity's *declared* type, which retains the placeholder
// AutoType sugar (deduced, pointing at the real closure type) rather than
// the plain deduced type -- this sugar was surviving into the splice's
// type-specifier reconstruction, where it correctly (if confusingly) tripped
// the ordinary "auto is not a valid alias target" check. Fixed by desugaring
// a deduced AutoType when resolving a type splice's reflected operand
// (Sema::BuildReflectionSpliceType, SemaReflect.cpp).
//
// The alias's identity must survive a second reflection round trip, including
// when its target is cv-qualified because the closure object is constexpr.

#include <meta>

using info = std::meta::info;

consteval bool test() {
  constexpr auto closure = [] {};
  constexpr auto cr = ^^decltype(closure);
  using ct = typename[:cr:];
  return is_type_alias(^^ct) && has_identifier(^^ct);
}
static_assert(test());

int main() {
  return 0;
}
