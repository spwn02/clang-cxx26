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
// ADDITIONAL_COMPILE_FLAGS: -freflection

// <experimental/reflection>
//
// [reflection]
//
// The operator-symbol tables must spell every compound assignment
// correctly: op_caret_equals used to duplicate plain "^" (upstream
// bloomberg/clang-p2996 issue #319 / PR #320).

#include <meta>

using namespace std::meta;

static_assert(symbol_of(operators::op_plus_equals) == "+=");
static_assert(symbol_of(operators::op_minus_equals) == "-=");
static_assert(symbol_of(operators::op_star_equals) == "*=");
static_assert(symbol_of(operators::op_slash_equals) == "/=");
static_assert(symbol_of(operators::op_percent_equals) == "%=");
static_assert(symbol_of(operators::op_caret_equals) == "^=");
static_assert(symbol_of(operators::op_ampersand_equals) == "&=");
static_assert(symbol_of(operators::op_pipe_equals) == "|=");
static_assert(symbol_of(operators::op_less_less_equals) == "<<=");
static_assert(symbol_of(operators::op_greater_greater_equals) == ">>=");

static_assert(u8symbol_of(operators::op_caret_equals) == u8"^=");

int main() {
  return 0;
}
