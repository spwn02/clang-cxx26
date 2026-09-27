// -*- C++ -*-
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// ADDITIONAL_COMPILE_FLAGS: -ffreestanding

// P1642R11 [utility.syn]: <utility> is entirely freestanding.

#include <utility>
#include <version>

#ifndef _LIBCPP_FREESTANDING
#  error "-ffreestanding must select libc++ freestanding mode"
#endif

#if !defined(__cpp_lib_freestanding_utility) || __cpp_lib_freestanding_utility != 202306L
#  error "missing or wrong __cpp_lib_freestanding_utility"
#endif

constexpr bool test() {
  int __first  = 1;
  int __second = 2;
  std::swap(__first, __second);
  auto __pair = std::make_pair(__first, __second);
  return __pair.first == 2 && __pair.second == 1;
}
static_assert(test());
