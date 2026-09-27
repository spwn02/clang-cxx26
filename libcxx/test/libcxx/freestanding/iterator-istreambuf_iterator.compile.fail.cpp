// -*- C++ -*-
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// ADDITIONAL_COMPILE_FLAGS: -ffreestanding

// P1642R11 [iterator.synopsis]: std::istreambuf_iterator stays hosted-only.

#include <iterator>

void test() {
  std::istreambuf_iterator<char> iterator;
  (void)iterator;
}
