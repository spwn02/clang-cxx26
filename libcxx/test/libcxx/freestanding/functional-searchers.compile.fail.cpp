// -*- C++ -*-
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// ADDITIONAL_COMPILE_FLAGS: -ffreestanding

// P1642R11 [functional.syn]: the searcher classes stay hosted-only.

#include <functional>

void test() {
  const char __s[] = "x";
  std::default_searcher __d(__s, __s + 1);
  std::boyer_moore_searcher __bm(__s, __s + 1);
  std::boyer_moore_horspool_searcher __bmh(__s, __s + 1);
  (void)__d;
  (void)__bm;
  (void)__bmh;
}
