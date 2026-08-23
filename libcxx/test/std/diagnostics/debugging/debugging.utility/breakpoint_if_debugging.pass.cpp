//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <debugging>

// void breakpoint() noexcept;
// void breakpoint_if_debugging() noexcept;
//
// [debugging.utility]p2: breakpoint_if_debugging() is
//   Effects: Equivalent to: if (is_debugger_present()) breakpoint();
//
// breakpoint() itself is intentionally not called here: with no debugger
// attached, its effect is an abnormal program termination (SIGTRAP), which
// this test can't portably recover from. Guarding the call on
// !is_debugger_present() below exercises breakpoint_if_debugging()'s
// documented Effects (the false branch) without ever reaching breakpoint().

#include <debugging>
#include <type_traits>

#include "test_macros.h"

int main(int, char**) {
  ASSERT_SAME_TYPE(decltype(std::breakpoint()), void);
  ASSERT_NOEXCEPT(std::breakpoint());
  ASSERT_SAME_TYPE(decltype(std::breakpoint_if_debugging()), void);
  ASSERT_NOEXCEPT(std::breakpoint_if_debugging());

  if (!std::is_debugger_present())
    std::breakpoint_if_debugging();

  return 0;
}
