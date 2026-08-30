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
// Regression test: a ReflectionSpliceType ('[:refl:]' used as a type) must
// survive a PCH round trip. This used to hit an llvm_unreachable in the
// generated AbstractTypeReader (see docs/LLVM22_SYNC.md, Milestone 5).
//
// RUN: %clang_cc1 -std=c++23 -freflection -emit-pch %s -o %t
// RUN: %clang_cc1 -std=c++23 -freflection -include-pch %t -verify %s

// expected-no-diagnostics

#ifndef HEADER
#define HEADER

using info = decltype(^^int);

using SplicedInt = [:^^int:];
constexpr SplicedInt non_dependent_value = 42;

// Dependent case: the ReflectionSpliceType is built with an
// underlying DependentTy until the template parameter is substituted.
template <info I>
constexpr bool is_int_sized() {
  return sizeof(typename [:I:]) == sizeof(int);
}

template <info I>
using SplicedFromParam = typename [:I:];

#else

static_assert(non_dependent_value == 42);

SplicedInt other = 7;

int use(SplicedInt v) { return v + other; }

static_assert(is_int_sized<^^int>());

SplicedFromParam<^^int> instantiated_after_pch = 5;

#endif
