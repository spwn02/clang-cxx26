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
// Required behavior: This function has no preconditions.
// Default behavior: implementation-defined.

#include <cassert>
#include <debugging>
#include <type_traits>

#include "test_macros.h"

int main(int, char**) {
  ASSERT_SAME_TYPE(decltype(std::is_debugger_present()), bool);
  ASSERT_NOEXCEPT(std::is_debugger_present());

  // No precondition: safe to call repeatedly, from anywhere, with no setup.
  // The result is implementation-defined -- under a lit test runner there is
  // ordinarily no debugger attached, but the standard doesn't guarantee
  // `false` here, so only the type/noexcept-ness is checked, not the value.
  (void)std::is_debugger_present();
  (void)std::is_debugger_present();

  return 0;
}
