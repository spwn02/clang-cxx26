//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17

// <span>

// P4144R1: span has no constructor from an initializer_list (P2447R6's constructor was removed for C++26): a braced
// list of values would dangle once the full-expression ends.

#include <span>

#include "test_macros.h"

void test() {
  std::span<const int> dynamic = {1, 2, 3};  // expected-error {{no matching constructor for initialization}}
  std::span<const int, 3> fixed = {1, 2, 3}; // expected-error {{no matching constructor for initialization}}
  (void)dynamic;
  (void)fixed;
}
