//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23, c++26

// <utility>

// P4206R0: string literals can no longer be wrapped, and a constant_wrapper whose second
// template argument is not the type of the value is ill-formed.
// (string_literal.pass.cpp shows the C++26 behaviour.)

#include <utility>

void test() {
  // A pointer to a string literal is not a valid template argument.
  auto s = std::cw<"abc">; // expected-error@*:* {{}}
  (void)s;

  // The second template argument must be decltype(X).
  std::constant_wrapper<1, long> w; // expected-error@*:* {{}}
  (void)w;
}
