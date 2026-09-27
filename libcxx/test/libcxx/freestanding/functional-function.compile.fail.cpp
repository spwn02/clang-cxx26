// -*- C++ -*-
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// ADDITIONAL_COMPILE_FLAGS: -ffreestanding

// P1642R11 [functional.syn]: std::function stays hosted-only.

#include <functional>

void test() {
  std::function<void()> f;
  (void)f;
}
