//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <complex>

// [cmplx.over]: the additional overloads of arg and proj are constexpr.

#include <complex>

static_assert(std::arg(3) == 0.0);
static_assert(std::arg(3.0f) == 0.0f);
static_assert(std::arg(3.0) == 0.0);
static_assert(std::arg(3.0L) == 0.0L);
static_assert(std::proj(3).real() == 3.0);
static_assert(std::proj(3.0f).real() == 3.0f);
