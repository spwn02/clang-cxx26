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

// Regression test: a file whose precompiled header includes a libc++ header
// (what a language server does with the includes that come before
// `import std;`) and that imports `std` asserted in the reader
// (markIncompleteDeclChain) and, without assertions, failed to compile
// std::println: the string_view of the header and the one of the module were two
// types (the preferred_name attribute of basic_string_view names string_view,
// and reading the attribute created the type of the class before it was merged
// with the one of the module), and the initializer of a constexpr static data
// member of a class template (std::basic_format_string) was instantiated by
// the instantiation of the class, as the definition in the module was demoted
// to a declaration.
//
// The test is run a second time with what a language server also does: the
// precompiled header is built without the bodies of the functions (the preamble
// of clangd), so its definition of std::basic_istream has no body for
// __inc_gcount and differs from the one of the module, which was read first and
// is in the global module fragment. -fskip-odr-check-in-gmf has to skip the
// check when the definition that is merged into is the one of the global module
// fragment, otherwise 'std::basic_istream has different definitions in
// different modules' is reported for std::cout (#327).
//
// This is a hand-rolled module build.

// RUN: mkdir %t
// RUN: echo "#include <iostream>" > %t/pre.h
// RUN: echo "import std;" > %t/use.cpp
// RUN: echo "int main() { std::println(\"x\"); std::cout << 1; std::vector<int> v; return (int)std::format(\"{}\", 1).size(); }" >> %t/use.cpp
// RUN: %{cxx} %{compile_flags} -std=c++26 \
// RUN:     -Wno-reserved-module-identifier -Wno-reserved-user-defined-literal \
// RUN:     --precompile -o %t/std.pcm -c %{module-dir}/std.cppm
// RUN: %{cxx} %{compile_flags} -std=c++26 \
// RUN:     -fmodule-file=std=%t/std.pcm -x c++-header -o %t/pre.pch %t/pre.h
// RUN: %{cxx} %{compile_flags} -std=c++26 \
// RUN:     -fmodule-file=std=%t/std.pcm -include-pch %t/pre.pch \
// RUN:     -fsyntax-only %t/use.cpp

// RUN: mkdir %t/nobodies
// RUN: %{cxx} %{compile_flags} -std=c++26 -Xclang -fskip-odr-check-in-gmf \
// RUN:     -fmodule-file=std=%t/std.pcm -x c++-header -Xclang -skip-function-bodies \
// RUN:     -o %t/nobodies/pre.pch %t/pre.h
// RUN: %{cxx} %{compile_flags} -std=c++26 -Xclang -fskip-odr-check-in-gmf \
// RUN:     -fmodule-file=std=%t/std.pcm -include-pch %t/nobodies/pre.pch \
// RUN:     -fsyntax-only %t/use.cpp
