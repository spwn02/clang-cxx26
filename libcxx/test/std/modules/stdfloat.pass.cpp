// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20
// UNSUPPORTED: clang-modules-build
// UNSUPPORTED: gcc
// XFAIL: has-no-cxx-module-support
// MODULE_DEPENDENCIES: std

import std;

#if _LIBCPP_STD_VER >= 23
#  if defined(__STDCPP_FLOAT16_T__)
std::float16_t value16;
#  endif
#  if defined(__STDCPP_FLOAT32_T__)
std::float32_t value32;
#  endif
#  if defined(__STDCPP_FLOAT64_T__)
std::float64_t value64;
#  endif
#  if defined(__STDCPP_FLOAT128_T__)
std::float128_t value128;
#  endif
#  if defined(__STDCPP_BFLOAT16_T__)
std::bfloat16_t value_bf16;
#  endif
#endif

int main(int, char**) { return 0; }
