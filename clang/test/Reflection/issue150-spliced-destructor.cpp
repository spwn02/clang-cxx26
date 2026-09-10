//===----------------------------------------------------------------------===//
//
// Copyright 2024 Bloomberg Finance L.P.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// RUN: %clang_cc1 -std=c++26 -freflection -verify %s
//
// Regression test for upstream bloomberg/clang-p2996 issue #150: an explicit
// destructor call naming its class type via a splice could not be spelled at
// all.
//
// The upstream report's own original repro used the *bare* splice-expression
// form ('value.~[:^^test:]();'), which a maintainer correctly confirmed is
// "works as intended": [expr.prim.splice]/2.1.2 explicitly makes a bare
// splice-expression ill-formed when it designates a destructor. That form
// must stay rejected -- see bare_splice_still_rejected below.
//
// The maintainer's own follow-up identified the *actual* bug: [class.dtor]
// p16 permits a "type-name, decltype-specifier, or computed-type-specifier"
// after the '~', and a splice-type-specifier ('typename[:R:]') is a
// computed-type-specifier ([dcl.type.splice]) -- so 'value.~typename[:R:]()'
// should work, but didn't. That's the form this test exercises and fixes.
//
// Two independent parser entry points needed the fix, both in
// clang/lib/Parse/ParseExprCXX.cpp: ParseUnqualifiedId's destructor-name
// case (used when the object's type is already known, e.g. non-dependent
// contexts) and ParseCXXPseudoDestructor (used when the object's type is
// still dependent at parse time, e.g. inside a function template -- see
// dependent_context below). Both already had an analogous, working
// 'decltype-specifier' case to mirror; splice support was added the same
// way in each. clang/lib/Sema/SemaExprCXX.cpp gained a new
// getDestructorTypeForSplice, mirroring the existing
// getDestructorTypeForDecltype's cross-check against the statically-known
// object type, for the same better-diagnostic reason -- see
// mismatched_type_diagnosed below.

struct test {};
struct other {};

void non_dependent_context(test& value) {
  value.~typename[:^^test:]();
}

template <typename T>
void dependent_context(T& value) {
  value.~typename[:^^T:]();
}
void instantiate_dependent_context(test& value) {
  dependent_context(value);
}

void mismatched_type_diagnosed(test& value) {
  value.~typename[:^^other:](); // expected-error {{destructor type 'other' in object destruction expression does not match the type 'test' of the object being destroyed}}
}

void bare_splice_still_rejected(test& value) {
  value.~[:^^test:](); // expected-error {{}}
}
