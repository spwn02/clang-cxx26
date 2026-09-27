// -*- C++ -*-
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20
// ADDITIONAL_COMPILE_FLAGS: -ffreestanding

#include <memory>

using _Shared = std::shared_ptr<int>;
static_assert(sizeof(std::out_ptr_t<_Shared, int*>) > 0);
static_assert(sizeof(std::inout_ptr_t<_Shared, int*>) > 0);
