// -*- C++ -*-
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// ADDITIONAL_COMPILE_FLAGS: -ffreestanding

// P1642R11 [tuple.syn]: <tuple> is entirely freestanding.

#include <tuple>
#include <version>

#ifndef _LIBCPP_FREESTANDING
#  error "-ffreestanding must select libc++ freestanding mode"
#endif

#if !defined(__cpp_lib_freestanding_tuple) || __cpp_lib_freestanding_tuple != 202306L
#  error "missing or wrong __cpp_lib_freestanding_tuple"
#endif

constexpr bool test() {
  auto __values = std::tuple{1, 2};
  return std::get<0>(__values) == 1 && std::get<1>(__values) == 2;
}
static_assert(test());
