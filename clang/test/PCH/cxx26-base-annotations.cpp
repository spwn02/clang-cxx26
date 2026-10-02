// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// RUN: %clang_cc1 -std=c++26 -freflection -fannotation-attributes -emit-pch -x c++-header %S/Inputs/cxx26-base-annotations.h -o %t.pch
// RUN: %clang_cc1 -std=c++26 -freflection -fannotation-attributes -include-pch %t.pch -verify %s
// RUN: %clang_cc1 -std=c++26 -freflection -fannotation-attributes -include-pch %t.pch -ast-dump-all -ast-dump-filter D1 %s | FileCheck %s --check-prefix=DUMP
// RUN: %clang_cc1 -std=c++26 -freflection -fannotation-attributes -ast-print -x c++ %S/Inputs/cxx26-base-annotations.h | FileCheck %s --check-prefix=PRINT
// expected-no-diagnostics

static_assert(value(base(^^D1), 0) == 13);
static_assert(value(base(^^D1), 1) == 21);
static_assert(value(base(^^D1), 2) == 13);
static_assert(annotation(base(^^D1), 3) == ^^sentinel);
static_assert(annotation(base(^^D1, 1), 0) == ^^sentinel);
static_assert(annotation(base(^^D1), 0) != annotation(base(^^D1), 2));
static_assert(value(base(^^Dep<2>), 0) == 2);
static_assert(value(base(^^Dep<3>), 0) == 3);
static_assert(value(base(^^Pack<B0, B1>), 0) == 1);
static_assert(value(base(^^Pack<B0, B1>, 1), 0) == 1);
static_assert(annotation(base(^^Pack<B0, B1>), 0) !=
              annotation(base(^^Pack<B0, B1>, 1), 0));

// DUMP: virtual public 'B0'
// DUMP: CXX26AnnotationAttr
// DUMP-NEXT: {{.*}}IntegerLiteral {{.*}} 13
// DUMP: CXX26AnnotationAttr
// DUMP-NEXT: {{.*}}IntegerLiteral {{.*}} 21
// DUMP: CXX26AnnotationAttr
// DUMP-NEXT: {{.*}}IntegerLiteral {{.*}} 13
// PRINT: struct D1 : {{\[\[=13\]\]}} {{\[\[=21\]\]}} {{\[\[=13\]\]}} virtual public B0, B1
// PRINT: template <int N> struct Dep : {{\[\[=N\]\]}} protected B0
// PRINT: template <class ...Bs> struct Pack : {{\[\[=1\]\]}} Bs...

static_assert(value(base(^^AnnotationPack<4, 5>), 0) == 4);
static_assert(value(base(^^AnnotationPack<4, 5>), 1) == 5);
static_assert(value(base(^^AnnotationPack<6, 7>), 0) == 6);
static_assert(value(base(^^AnnotationPack<6, 7>), 1) == 7);
static_assert(annotation(base(^^AnnotationPack<6, 7>), 2) == ^^sentinel);
static_assert(annotation(base(^^AnnotationPack<>), 0) == ^^sentinel);

static_assert(is_annotation(annotation(base(^^D1), 0)));
static_assert([:constant(annotation(base(^^D1), 0)):] == 13);
static_assert([:constant(annotation(base(^^Dep<3>), 0)):] == 3);
