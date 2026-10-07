//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: clang-modules-build, gcc, no-threads
// XFAIL: has-no-cxx-module-support
// MODULE_DEPENDENCIES: std

// The names added by P4052R0 (saturating_*) are exported.

import std;

int main(int, char**) {
  // P4052R0
  if (std::saturating_add(std::uint8_t{250}, std::uint8_t{10}) != 255)
    return 1;
  if (std::saturating_cast<std::uint8_t>(1000) != 255)
    return 2;
  if (std::saturating_sub(std::uint8_t{3}, std::uint8_t{10}) != 0 || std::saturating_mul(std::int8_t{100}, std::int8_t{2}) != 127 ||
      std::saturating_div(std::int8_t{-128}, std::int8_t{-1}) != 127)
    return 3;
  return 0;
}
