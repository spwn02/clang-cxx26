// -*- C++ -*-
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20
// ADDITIONAL_COMPILE_FLAGS: -ffreestanding

#include <mdspan>

#include "test_macros.h"

#if TEST_STD_VER >= 26 && !defined(__cpp_lib_freestanding_mdspan)
#  error "missing freestanding mdspan feature-test macro"
#endif

int main(int, char**) {
  int __values[4] = {};
  std::mdspan<int, std::extents<int, 2, 2>> __view(__values);
  __view[1, 0] = 42;
  return __view[1, 0] != 42;
}
