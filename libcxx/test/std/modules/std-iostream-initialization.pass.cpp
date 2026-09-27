//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, clang-modules-build
// UNSUPPORTED: gcc
// XFAIL: has-no-cxx-module-support

// LWG3878: importing std initializes the standard iostream objects.
// MODULE_DEPENDENCIES: std

import std;

int main(int, char**) {
  std::cout << "initialized\n";
  return std::cout.good() ? 0 : 1;
}
