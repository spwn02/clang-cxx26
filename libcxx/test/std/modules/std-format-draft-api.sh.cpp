//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: clang-modules-build
// UNSUPPORTED: gcc

// XFAIL: has-no-cxx-module-support

// RUN: mkdir %t
// RUN: %{cxx} %{compile_flags} -std=c++26 -freflection-latest \
// RUN:     -Wno-reserved-module-identifier -Wno-reserved-user-defined-literal \
// RUN:     --precompile -o %t/std.pcm -c %{module-dir}/std.cppm
// RUN: %{cxx} %{compile_flags} %{link_flags} -std=c++26 -freflection-latest \
// RUN:     -fmodule-file=std=%t/std.pcm %t/std.pcm \
// RUN:     %s -o %t/std-format-draft-api.sh.cpp.tsk
// RUN: %{exec} %t/std-format-draft-api.sh.cpp.tsk

// [format.syn]: the facilities of <format> that the current draft requires, as seen through `import std;`.
#include <cassert>

#include "test_macros.h"

import std;

int main(int, char**) {
  // P3953R3: dynamic_format (the former runtime_format).
  assert(std::format(std::dynamic_format("{}"), 42) == "42");
  assert(std::format(std::dynamic_format("{:?}"), "x") == "\"x\"");
#ifndef TEST_HAS_NO_WIDE_CHARACTERS
  assert(std::format(L"{}", 1) == L"1");
  assert(std::format(std::dynamic_format(L"{}"), 7) == L"7");
#endif
  return 0;
}
