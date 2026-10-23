//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// The [cpp.predefined] __STDCPP_FLOAT<N>_T__ macros are defined by the frontend only for libc++ users.
// ADDITIONAL_COMPILE_FLAGS: -Xclang -fstdcpp-float-macros

// <cmath>

// The overloads for the extended floating-point types of frexp, modf, remquo and nexttoward are constexpr.

#include <cmath>
#include <stdfloat>

#if defined(__STDCPP_FLOAT32_T__) && defined(__STDCPP_FLOAT64_T__)
constexpr bool test() {
  int e;
  std::float64_t fr = std::frexp(std::float64_t(8), &e);
  std::float32_t ip;
  std::modf(std::float32_t(2.5f), &ip);
  int q;
  std::remquo(std::float32_t(5), std::float32_t(3), &q);
  return fr == 0.5 && e == 4 && std::nexttoward(std::float32_t(1), 2.0L) > std::float32_t(1);
}
static_assert(test());
#endif
