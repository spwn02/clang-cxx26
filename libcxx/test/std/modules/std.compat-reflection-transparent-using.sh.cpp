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

// This test builds matching std and std.compat PCMs for each language mode.
// Do not add MODULE_DEPENDENCIES here: that injects the default-mode PCMs
// alongside the separately built C++29 modules.

// P4466R2 [namespace.udecl]: these exported declarations inhabit global
// namespace scope and name global declarations. The global target scope is
// equal to the target scope, with no intervening scopes, so both using-
// declarators are reflection-transparent.

// RUN: mkdir %t
// RUN: %{cxx} %{compile_flags} -std=c++26 \
// RUN:     -Wno-reserved-module-identifier -Wno-reserved-user-defined-literal \
// RUN:     --precompile -o %t/std.pcm -c %{module-dir}/std.cppm
// RUN: %{cxx} %{compile_flags} -std=c++26 \
// RUN:     -Wno-reserved-module-identifier -Wno-reserved-user-defined-literal \
// RUN:     -fmodule-file=std=%t/std.pcm --precompile -o %t/std.compat.pcm \
// RUN:     -c %{module-dir}/std.compat.cppm
// RUN: %{cxx} %{compile_flags} -std=c++26 -fsyntax-only \
// RUN:     -fmodule-file=std=%t/std.pcm \
// RUN:     -fmodule-file=std.compat=%t/std.compat.pcm \
// RUN:     %s
// RUN: %{cxx} %{compile_flags} -std=c++29 \
// RUN:     -Wno-reserved-module-identifier -Wno-reserved-user-defined-literal \
// RUN:     --precompile -o %t/std.pcm -c %{module-dir}/std.cppm
// RUN: %{cxx} %{compile_flags} -std=c++29 \
// RUN:     -Wno-reserved-module-identifier -Wno-reserved-user-defined-literal \
// RUN:     -fmodule-file=std=%t/std.pcm --precompile -o %t/std.compat.pcm \
// RUN:     -c %{module-dir}/std.compat.cppm
// RUN: %{cxx} %{compile_flags} -std=c++29 -fsyntax-only \
// RUN:     -fmodule-file=std=%t/std.pcm \
// RUN:     -fmodule-file=std.compat=%t/std.compat.pcm \
// RUN:     %s

import std.compat;

static_assert(^^::size_t == ^^size_t);
static_assert(^^::strlen == ^^strlen);
static_assert(std::meta::identifier_of(^^::size_t) == "size_t");
static_assert(std::meta::identifier_of(^^::strlen) == "strlen");

int main(int, char**) { return ::strlen("module") != 6; }
