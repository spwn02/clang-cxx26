// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// RUN: %clang_cc1 -std=c++26 -freflection -fsyntax-only -verify %s

// expected-no-diagnostics

// define_aggregate of a specialization whose template argument is a null
// pointer value asserted ("unimplemented") when the specialization was
// rebuilt: the argument was one of the kinds that were not handled.

using info = decltype(^^int);
using size_t = __SIZE_TYPE__;
consteval info spec(info type, const char* name, size_t length) {
  return __metafunction(90, type, name != nullptr, length, ^^const char, static_cast<const char*>(name),
                       false, 0, false, 0, false,
                       size_t(0), (const info*)nullptr, ^^spec);
}
consteval info define(info type, const info* fields, size_t count) {
  return __metafunction(91, type, count, static_cast<const info*>(fields));
}

template <decltype(nullptr) N> struct S;
consteval {
  info fields[] = {spec(^^int, "x", 1)};
  define(^^S<nullptr>, fields, 1);
}
static_assert(__is_complete_type(S<nullptr>));
static_assert(S<nullptr>{3}.x == 3);

template <int *P, class T> struct U;
consteval {
  info fields[] = {spec(^^int, "y", 1)};
  define(^^U<nullptr, int>, fields, 1);
}
static_assert(U<nullptr, int>{4}.y == 4);
