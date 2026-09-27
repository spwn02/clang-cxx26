//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

#include <cassert>
#include <cmath>
#include <complex>
#include <stdfloat>

int main(int, char**) {
#if defined(__STDCPP_FLOAT128_T__) && defined(__GLIBC__)
  using F = std::float128_t;
  const F a = 1.5F128;
  const F b = 2.5F128;
  assert(std::abs(static_cast<double>(std::hypot(a, b)) - std::hypot(1.5, 2.5)) < 1e-12);
  assert(static_cast<double>(std::sin(a)) > 0.99);
  std::complex<F> z(3, 4);
  const auto root = std::sqrt(z);
  assert(static_cast<double>(root.real()) > 1.9);
  assert(static_cast<double>(root.imag()) > 0.9);
  assert(static_cast<double>(std::hypot(z.real(), z.imag())) == 5.0);
#endif
  return 0;
}
