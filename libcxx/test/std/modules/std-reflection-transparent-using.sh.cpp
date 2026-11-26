//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20
// UNSUPPORTED: clang-modules-build
// UNSUPPORTED: gcc
// XFAIL: has-no-cxx-module-support

// P4466R2 [namespace.udecl]: the std module's using-declarators inhabit a
// namespace-associated scope. For declarations in libc++'s inline namespace
// std::__1, std contains the target scope and the only intervening scope is
// inline, so these using-declarators are reflection-transparent.

// RUN: mkdir %t
// RUN: %{cxx} %{compile_flags} -std=c++26 \
// RUN:     -Wno-reserved-module-identifier -Wno-reserved-user-defined-literal \
// RUN:     --precompile -o %t/std.pcm -c %{module-dir}/std.cppm
// RUN: %{cxx} %{compile_flags} -std=c++26 -fsyntax-only \
// RUN:     -Xclang -verify -fmodule-file=std=%t/std.pcm %s
// RUN: %{cxx} %{compile_flags} -std=c++29 \
// RUN:     -Wno-reserved-module-identifier -Wno-reserved-user-defined-literal \
// RUN:     --precompile -o %t/std.pcm -c %{module-dir}/std.cppm
// RUN: %{cxx} %{compile_flags} -std=c++29 -fsyntax-only \
// RUN:     -Xclang -verify -fmodule-file=std=%t/std.pcm %s

import std;

static_assert(std::meta::is_template(^^std::basic_string));
static_assert(std::meta::is_template(^^std::to_underlying));
static_assert(std::meta::identifier_of(^^std::basic_string) == "basic_string");
static_assert(std::meta::is_namespace(
    std::meta::parent_of(^^std::basic_string)));
static_assert(std::meta::identifier_of(
    std::meta::parent_of(^^std::basic_string)) == "__1");

using ReflectedString = typename [: ^^std::string :];
static_assert(std::is_same_v<ReflectedString, std::string>);

// std::swap and std::to_string are each imported by transparent
// using-declarators, but each name denotes an overload set. [expr.reflect]/5
// still rejects reflection of an overload set after applying P4466R2.
constexpr auto swap_overload_set = ^^std::swap; // expected-error {{cannot take the reflection of an overload set}}
constexpr auto to_string_overload_set = ^^std::to_string; // expected-error {{cannot take the reflection of an overload set}}

int main(int, char**) { return 0; }
