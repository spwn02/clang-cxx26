// -*- C++ -*-
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// ADDITIONAL_COMPILE_FLAGS: -ffreestanding

// P1642R11 [ratio.syn]: <ratio> is entirely freestanding.

#include <ratio>
#include <version>

#ifndef _LIBCPP_FREESTANDING
#  error "-ffreestanding must select libc++ freestanding mode"
#endif

#if !defined(__cpp_lib_freestanding_ratio) || __cpp_lib_freestanding_ratio != 202306L
#  error "missing or wrong __cpp_lib_freestanding_ratio"
#endif

using __sum = std::ratio_add<std::ratio<1, 3>, std::ratio<1, 6>>;
static_assert(__sum::num == 1 && __sum::den == 2);
static_assert(std::ratio_less_v<std::milli, std::ratio<1>>);
