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
// NOTE: this covers the primary (previously hard-error) symptom only. A
// second, deeper symptom from the same issue report --
// 'is_type_alias(^^ct)' evaluating to false even once the alias declares
// successfully, i.e. the alias's own identity not surviving a *second*
// reflection round-trip through '^^ct' -- remains open; not yet root-caused
// despite extensive tracing (every layer from CXXReflectExpr construction
// through APValue's reflected-type accessor was verified to correctly
// preserve the TypedefType sugar, so the loss happens somewhere not yet
// located). Do not add a static_assert(is_type_alias(...)) call here until
// that's fixed -- see the Reflection Closeup epic's tracker for the current
// investigation state.

#include <meta>

using info = std::meta::info;

consteval bool test() {
  constexpr auto closure = [] {};
  constexpr auto cr = ^^decltype(closure);
  using ct = typename[:cr:];
  return true;
}
static_assert(test());

int main() {
  return 0;
}
