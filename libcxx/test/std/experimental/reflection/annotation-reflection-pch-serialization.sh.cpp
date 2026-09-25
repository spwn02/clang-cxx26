//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

// A reflection of an annotation used as a template argument (or stored in an
// instantiated template) was written to a PCH/module without its payload and
// read back as a plain, non-reflection APValue: the instantiation then failed to
// evaluate, and tools that walk the AST of the importing translation unit
// (clang-tidy's ParentMap) crashed in RecursiveASTVisitor::TraverseCXXReflectExpr.
// Spwn02/clang-cxx26#121.

// RUN: %{cxx} %{flags} %{compile_flags} -std=c++26 -freflection-latest \
// RUN:     -x c++-header %s -o %t.pch
// RUN: %{cxx} %{flags} %{compile_flags} -std=c++26 -freflection-latest \
// RUN:     -include-pch %t.pch -fsyntax-only %s -DUSE

// expected-no-diagnostics

#ifndef USE

#include <meta>

[[=42]] void annotated();

template <std::meta::info R>
struct Holder {
  static consteval bool is_annotation() { return std::meta::is_annotation(R); }
  static consteval bool has_value_42() {
    return std::meta::constant_of(R) == std::meta::reflect_constant(42);
  }
};

constexpr auto annotation = std::meta::annotations_of(^^annotated)[0];
using HeaderHolder = Holder<annotation>;
static_assert(HeaderHolder::is_annotation());

#else

static_assert(HeaderHolder::is_annotation());
static_assert(HeaderHolder::has_value_42());
static_assert(Holder<annotation>::is_annotation());

int main(int, char**) { return 0; }

#endif
