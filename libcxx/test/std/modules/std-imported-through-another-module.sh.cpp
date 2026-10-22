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

// Regression test: a file that imports a module that imports `std` (and not
// `std` itself) and names something that `std` does not export ran for ever.
// The module std has more than one declaration of such a name (the
// using-declaration of the global module fragment and the ones of the module
// purview, e.g. std::llrint of <cmath> and of <simd>): the most recent
// declaration of the chain that the reader attached first was replaced by the
// first declaration of the chain that it attached second, and the correction of
// the typo of `std::lrint` went round that declaration for ever.
//
// This is a hand-rolled module build.

// RUN: mkdir %t
// RUN: echo "export module Mod;" > %t/Mod.cppm
// RUN: echo "import std;" >> %t/Mod.cppm
// RUN: echo "import Mod;" > %t/use.cpp
// RUN: echo "int main() { return (int)std::lrint(1.0); }" >> %t/use.cpp
// RUN: %{cxx} %{compile_flags} -std=c++26 \
// RUN:     -Wno-reserved-module-identifier -Wno-reserved-user-defined-literal \
// RUN:     --precompile -o %t/std.pcm -c %{module-dir}/std.cppm
// RUN: %{cxx} %{compile_flags} -std=c++26 \
// RUN:     -fmodule-file=std=%t/std.pcm --precompile -o %t/Mod.pcm %t/Mod.cppm
// RUN: not %{cxx} %{compile_flags} -std=c++26 \
// RUN:     -fmodule-file=std=%t/std.pcm -fmodule-file=Mod=%t/Mod.pcm \
// RUN:     -fsyntax-only %t/use.cpp > %t/out.txt 2>&1
// RUN: grep "lrint" %t/out.txt
