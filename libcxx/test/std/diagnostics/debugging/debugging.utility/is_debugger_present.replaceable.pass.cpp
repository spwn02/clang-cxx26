//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <debugging>

// bool is_debugger_present() noexcept;
//
// [debugging.utility]p5 (via P2810R4): "This function is replaceable
// ([dcl.fct.def.replace])." Test that a program-supplied definition
// displaces the library's default, per [replacement.functions].

#include <cassert>
#include <debugging>

bool std::is_debugger_present() noexcept { return true; }

int main(int, char**) {
  assert(std::is_debugger_present() == true);
  return 0;
}
