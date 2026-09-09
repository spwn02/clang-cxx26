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
// expected-no-diagnostics

// <experimental/reflection>
//
// [reflection]
//
// Regression test for upstream bloomberg/clang-p2996 issue #346: a type
// that is itself an rvalue reference (e.g. via decltype on an xvalue
// expression like std::move(1)) must not spuriously warn that '&&' binds
// to the reflect operand -- no '&&' token is actually adjacent to the
// reflect operator's parsed operand here, the reference-ness comes only
// from decltype's value-category deduction.

#include <meta>
#include <utility>

static_assert(^^decltype(std::move(1)) != ^^int);

int main() {
  return 0;
}
