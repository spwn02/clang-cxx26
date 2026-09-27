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
// complex.members: explicit is false iff destination floating-point
// conversion rank is greater than or equal to the source rank.
static_assert(std::is_convertible_v<std::complex<std::float32_t>, std::complex<std::float64_t>>);
static_assert(!std::is_convertible_v<std::complex<std::float64_t>, std::complex<std::float32_t>>);

template <class T>
void test_arithmetic(T one, T two, T tolerance) {
  using C = std::complex<T>;
  C x(one, two), y(two, one);
  assert((x + y).real() == T(one + two));
  assert((x + y).imag() == T(two + one));
  assert((x - y).real() == T(one - two));
  assert((x * y).real() == T(one * two - two * one));
  assert((x * y).imag() == T(one * one + two * two));
  assert((x / C(one, T(0))).real() == one);
  assert((x / C(one, T(0))).imag() == two);
  assert((x + one).real() == T(one + one));
  assert((one + x).imag() == two);
  assert((x - one).real() == T(0));
  assert((one - x).imag() == T(-two));
  assert((x * two).real() == T(one * two));
  assert((two * x).imag() == T(two * two));
  assert((x / two).real() == T(one / two));
  assert((two / C(one, T(0))).real() == T(two));
  assert(std::abs(std::abs(x) - std::sqrt(one * one + two * two)) <= tolerance);
  assert(std::abs(std::arg(C(one, T(0)))) <= tolerance);
  assert(std::norm(x) == T(one * one + two * two));
  assert(std::conj(x).imag() == T(-two));
  assert(std::proj(x).real() == one);
  C polar = std::polar(std::abs(x), std::arg(x));
  assert(std::abs(polar.real() - one) <= tolerance);
  assert(std::abs(polar.imag() - two) <= tolerance);
}

template <class T>
void test_transcendentals(T real, T imag) {
  std::complex<T> value(real, imag);
  (void)std::exp(value);
  (void)std::log(value);
  (void)std::log10(value);
  (void)std::pow(value, T(2));
  (void)std::pow(value, value);
  (void)std::sqrt(value);
  (void)std::sin(value);
  (void)std::cos(value);
  (void)std::tan(value);
  (void)std::asin(value);
  (void)std::acos(value);
  (void)std::atan(value);
  (void)std::sinh(value);
  (void)std::cosh(value);
  (void)std::tanh(value);
  (void)std::asinh(value);
  (void)std::acosh(value);
  (void)std::atanh(value);
}

int main(int, char**) {
  std::complex<std::float64_t> widened = std::complex<std::float32_t>(1, 2);
  std::complex<std::float32_t> narrowed(std::complex<std::float64_t>(1, 2));
  assert(widened.real() == 1.0 && narrowed.imag() == 2.0f);
  test_arithmetic(std::float32_t(1), std::float32_t(2), std::float32_t(0.0001));
  test_arithmetic(std::float64_t(1), std::float64_t(2), std::float64_t(0.0000001));
  test_arithmetic(std::float16_t(1), std::float16_t(2), std::float16_t(0.01));
  test_arithmetic(std::bfloat16_t(1), std::bfloat16_t(2), std::bfloat16_t(0.02));
  test_transcendentals(std::float32_t(1), std::float32_t(2));
  test_transcendentals(std::float64_t(1), std::float64_t(2));
  test_transcendentals(std::float16_t(1), std::float16_t(2));
  test_transcendentals(std::bfloat16_t(1), std::bfloat16_t(2));
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
