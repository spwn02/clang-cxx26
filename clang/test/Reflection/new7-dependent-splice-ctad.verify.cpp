//===----------------------------------------------------------------------===//
//
// Copyright 2026 Bloomberg Finance L.P.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// RUN: %clang_cc1 %s -std=c++23 -freflection -verify
//
// [dcl.type.simple]p3 / [temp.dep.splice]: a dependent splice-specifier used
// without an explicit 'typename' keyword cannot stand for a placeholder for
// a deduced class type -- CWG3003's restriction on dependent
// nested-name-specifiers in class template argument deduction extends to
// dependent splices. This is the "NEW-7" item from the Reflection Closeup
// epic's tracker: a dependent, implicit (no 'typename') splice-specifier
// used in copy-list-initialization position was previously wrongly accepted
// as though it deduced a class template specialization.
//
// Getting this right requires ReflectionSpliceType::getTypenameKWLoc()'s
// validity to reliably distinguish "no explicit typename" from "explicit
// typename" per declaration, which needed three separate upstream fixes
// (see Sema::AddInitializerToDecl's comment in SemaDecl.cpp for the full
// history): SemaType.cpp's TST_type_splice case no longer substitutes the
// splice's own location for a genuinely absent keyword location,
// ParseOptionalCXXScopeSpecifier's 'typename [:R:]' rewrite now threads the
// real keyword location through instead of discarding it, and
// DependentReflectionSpliceType::Profile now folds in whether an explicit
// keyword was present -- otherwise two structurally-identical splice
// operands (e.g. the same depth/index template parameter used by two
// different function templates) silently collapsed onto the same cached
// type node regardless of which one had 'typename' written.

using info = decltype(^^int);

namespace non_dependent {
template <typename T> struct TCls { T value; };
template <typename T> TCls(T) -> TCls<T>;
}  // namespace non_dependent

// The forbidden form: a dependent, implicit splice in copy-list-init.
template <info R> consteval auto Forbidden(int value) {
  [:R:] obj = {value};
  // expected-error@-1 {{a dependent splice-specifier used without 'typename' cannot be used to deduce a class template specialization from copy-list-initialization}}
  return obj;
}

// Positive controls -- none of these should be diagnosed.
template <info R> consteval auto AllowedExplicitTypename(int value) {
  typename [:R:] obj = {value};
  return obj;
}
template <info R> consteval auto AllowedDirectListInit(int value) {
  [:R:] obj{value};
  return obj;
}
template <info R> consteval auto AllowedParenInit(int value) {
  [:R:] obj(value);
  return obj;
}
template <info R> consteval auto AllowedNonListCopyInit(non_dependent::TCls<int> value) {
  [:R:] obj = value;
  return obj;
}

static_assert(AllowedExplicitTypename<^^non_dependent::TCls>(1).value == 1);
static_assert(AllowedDirectListInit<^^non_dependent::TCls>(2).value == 2);
static_assert(AllowedParenInit<^^non_dependent::TCls>(3).value == 3);
static_assert(AllowedNonListCopyInit<^^non_dependent::TCls>({4}).value == 4);

// A non-dependent splice in the same (implicit, copy-list-init) shape is
// unaffected -- the restriction is specific to dependent splices.
constexpr [:^^non_dependent::TCls:]<int> non_dependent_obj = {5};
static_assert(non_dependent_obj.value == 5);

void instantiate() {
  (void)Forbidden<^^non_dependent::TCls>(6);
}
