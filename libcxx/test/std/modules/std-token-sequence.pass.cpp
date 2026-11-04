//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23, c++26
// UNSUPPORTED: clang-modules-build, gcc, no-threads
// XFAIL: has-no-cxx-module-support
// MODULE_DEPENDENCIES: std

// The class std::meta::token_sequence (P4380) through the std module: a
// sequence built in the importing translation unit works, including the
// operations the compiler does natively. (Token contents that cross a module
// boundary are not preserved: that is a separate limitation.)

import std;

consteval bool f() {
  std::meta::token_sequence s = ^^{ a b };
  s += ^^{ c };
  std::meta::token t = s[1];
  return s.size() == 3 && s[1] == ^^{ b } && s.subseq(1).size() == 2 && t == ^^{ b } &&
         (s + ^^{ d }).size() == 4 && s == ^^{ a b c };
}
static_assert(f());

int main(int, char**) { return 0; }
