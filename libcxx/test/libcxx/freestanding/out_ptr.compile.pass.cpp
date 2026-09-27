// -*- C++ -*-
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20
// ADDITIONAL_COMPILE_FLAGS: -ffreestanding

#include <memory>

#include "test_macros.h"

int main(int, char**) {
  int* __raw = nullptr;
  std::unique_ptr<int> __ptr;
  {
    auto __out_handle = std::out_ptr(__ptr);
    *static_cast<int**>(__out_handle) = __raw;
  }
  {
    auto __inout = std::inout_ptr(__ptr);
    *static_cast<int**>(__inout) = __raw;
  }
  return 0;
}
