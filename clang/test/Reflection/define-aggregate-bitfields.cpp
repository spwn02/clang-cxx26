// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// RUN: %clang_cc1 -std=c++26 -freflection -fsyntax-only -verify -verify-ignore-unexpected=note %s
// RUN: %clang_cc1 -std=c++26 -freflection -ast-print -DPRINT %s | FileCheck %s

using info = decltype(^^int);
using size_t = __SIZE_TYPE__;
// Intrinsic IDs match the table in ExprConstantMeta.cpp.
consteval info spec(info type, const char* name, size_t length, bool bitfield = false, int width = 0) {
  return __metafunction(96, type, name != nullptr, length, ^^const char, static_cast<const char*>(name),
                       false, 0, bitfield, width, false,
                       size_t(0), (const info*)nullptr,
                       size_t(0), (const info*)nullptr, ^^spec);
}
consteval info define(info type, const info* fields, size_t count) {
  return __metafunction(99, type, count, static_cast<const info*>(fields));
}
struct A;
consteval {
  info fields[] = {spec(^^int, nullptr, 0, true, 3), spec(^^int, "x", 1)};
  define(^^A, fields, 2);
}
// CHECK: struct A {
// CHECK-NEXT: int : 3UL;
// CHECK-NEXT: int x;
struct Expected { int : 3; int x; };
static_assert(sizeof(A) == sizeof(Expected));
constexpr A a{1};
static_assert(a.x == 1);
#ifndef PRINT
void access(A& a) { (void)a.__0; } // expected-error {{no member named '__0' in 'A'}}
#endif
struct B;
consteval {
  info fields[] = {spec(^^int, "_", 1), spec(^^int, "_", 1)};
  define(^^B, fields, 2);
}
constexpr B b{1, 2};
#ifndef PRINT
struct Qualified;
consteval { // expected-error {{evaluating expression of a consteval block must be a constant expression}}
  define(^^const Qualified, nullptr, 0); // expected-note {{expected a reflection of a cv-unqualified class type}}
}
struct AliasQualified;
using ConstClass = const AliasQualified;
consteval { // expected-error {{evaluating expression of a consteval block must be a constant expression}}
  define(^^ConstClass, nullptr, 0); // expected-note {{expected a reflection of a cv-unqualified class type}}
}
struct Incomplete;
struct WithIncomplete;
consteval { // expected-error {{evaluating expression of a consteval block must be a constant expression}}
  info fields[] = {spec(^^Incomplete, "x", 1)};
  define(^^WithIncomplete, fields, 1); // expected-note {{cannot introspect the size of an incomplete type}}
}
#endif
