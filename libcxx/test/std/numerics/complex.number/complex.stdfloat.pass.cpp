//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

#include <complex>
#include <cassert>
#include <stdfloat>
#include <type_traits>

#include "test_macros.h"

static_assert(sizeof(std::complex<float>) == 2 * sizeof(float));
static_assert(alignof(std::complex<float>) == alignof(float));
static_assert(sizeof(std::complex<double>) == 2 * sizeof(double));
static_assert(alignof(std::complex<double>) == alignof(double));
static_assert(sizeof(std::complex<long double>) == 2 * sizeof(long double));
static_assert(alignof(std::complex<long double>) == alignof(long double));

int main(int, char**) {
  std::complex<std::float32_t> x(1.0f, 2.0f);
  std::complex<std::float32_t> y(2.0f, 1.0f);
  auto sum = x + y;
  auto product = x * y;
  assert(sum.real() == 3.0f && sum.imag() == 3.0f);
  assert(product.real() == 0.0f && product.imag() == 5.0f);
  assert(std::abs(x) > 2.2f && std::abs(x) < 2.3f);
  auto z = std::polar(std::abs(x), std::arg(x));
  assert(std::abs(z - x) < 0.0001f);

  std::complex<std::float64_t> d(3.0, 4.0);
  assert(std::abs(d) == 5.0);

  std::complex<std::float16_t> h(1.0f, 2.0f);
  assert((h + h).real() == 2.0f);
  return 0;
}
