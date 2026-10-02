// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// RUN: clang-import-test -Xcc=-x -Xcc=c++ -Xcc=-std=c++26 -Xcc=-freflection -Xcc=-fannotation-attributes -dump-ast -import %S/Inputs/Base.cpp -expression %s | FileCheck %s

using info = decltype(^^int);
struct sentinel {};
consteval info base(info r) {
  return __metafunction(2, r, ^^sentinel, 0, ^^base);
}
consteval info annotation(info r, unsigned i) {
  return __metafunction(114, r, ^^sentinel, i, ^^annotation);
}
consteval int value(info r, unsigned i) {
  return __metafunction(25, ^^int, annotation(r, i), ^^value);
}

static_assert(value(base(^^D), 0) == 13);
static_assert(value(base(^^D), 1) == 21);
static_assert(value(base(^^D), 2) == 13);
static_assert(annotation(base(^^D), 3) == ^^sentinel);
static_assert(annotation(base(^^D), 0) != annotation(base(^^D), 2));

// CHECK: CXXRecordDecl {{.*}} struct D definition
// CHECK: virtual public 'B'
// CHECK: CXX26AnnotationAttr
// CHECK-NEXT: {{.*}}IntegerLiteral {{.*}} 13
// CHECK: CXX26AnnotationAttr
// CHECK-NEXT: {{.*}}IntegerLiteral {{.*}} 21
// CHECK: CXX26AnnotationAttr
// CHECK-NEXT: {{.*}}IntegerLiteral {{.*}} 13
