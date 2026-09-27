//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

// <cmath>

#include <cassert>
#include <cmath>
#include <stdfloat>
#include <type_traits>

template <class Real>
void test() {
  assert(std::hermite(2, Real(0.5)) == Real(-1));
  assert(std::legendre(2, Real(0.5)) == Real(-0.125));
  assert(std::laguerre(2, Real(0.5)) == Real(0.125));
  assert(std::assoc_laguerre(2, 1, Real(0.5)) == Real(1.625));
  assert(std::assoc_legendre(1, 0, Real(0.5)) == Real(0.5));
  Real beta_value = std::beta(Real(2), Real(3));
  assert(beta_value > Real(0.083) && beta_value < Real(0.084));
  Real spherical_value = std::sph_legendre(0, 0, Real(1));
  assert(spherical_value > Real(0.28) && spherical_value < Real(0.29));
  assert(std::comp_ellint_1(Real(0)) > Real(1.5) && std::comp_ellint_1(Real(0)) < Real(1.7));
  assert(std::comp_ellint_2(Real(0)) > Real(1.5) && std::comp_ellint_2(Real(0)) < Real(1.7));
  assert(std::comp_ellint_3(Real(0), Real(0)) > Real(1.5) && std::comp_ellint_3(Real(0), Real(0)) < Real(1.7));
  Real incomplete_first = std::ellint_1(Real(0), Real(0.5));
  Real incomplete_second = std::ellint_2(Real(0), Real(0.5));
  Real incomplete_third = std::ellint_3(Real(0), Real(0), Real(0.5));
  assert(incomplete_first > Real(0.49) && incomplete_first < Real(0.51));
  assert(incomplete_second > Real(0.49) && incomplete_second < Real(0.51));
  assert(incomplete_third > Real(0.49) && incomplete_third < Real(0.51));
  Real exponential_integral = std::expint(Real(1));
  assert(exponential_integral > Real(1.85) && exponential_integral < Real(1.95));
  Real zeta_value = std::riemann_zeta(Real(2));
  assert(zeta_value > Real(1.6) && zeta_value < Real(1.7));
  Real bessel_j = std::cyl_bessel_j(Real(0), Real(1));
  Real neumann_y = std::cyl_neumann(Real(0), Real(1));
  Real bessel_i = std::cyl_bessel_i(Real(0), Real(1));
  Real bessel_k = std::cyl_bessel_k(Real(0), Real(1));
  assert(bessel_j > Real(0.7) && bessel_j < Real(0.8));
  assert(neumann_y > Real(0.05) && neumann_y < Real(0.12));
  assert(bessel_i > Real(1.2) && bessel_i < Real(1.4));
  assert(bessel_k > Real(0.35) && bessel_k < Real(0.5));
  Real spherical_bessel = std::sph_bessel(0, Real(1));
  Real spherical_neumann = std::sph_neumann(0, Real(1));
  assert(spherical_bessel > Real(0.8) && spherical_bessel < Real(0.9));
  assert(spherical_neumann > Real(-0.7) && spherical_neumann < Real(-0.4));
  static_assert(std::is_same_v<decltype(std::hermite(2, Real(0.5))), Real>);
  static_assert(std::is_same_v<decltype(std::legendre(2, Real(0.5))), Real>);
  static_assert(std::is_same_v<decltype(std::laguerre(2, Real(0.5))), Real>);
  static_assert(std::is_same_v<decltype(std::assoc_laguerre(2, 1, Real(0.5))), Real>);
  static_assert(std::is_same_v<decltype(std::assoc_legendre(1, 0, Real(0.5))), Real>);
  static_assert(std::is_same_v<decltype(std::beta(Real(2), Real(3))), Real>);
  static_assert(std::is_same_v<decltype(std::sph_legendre(0, 0, Real(1))), Real>);
  static_assert(std::is_same_v<decltype(std::comp_ellint_1(Real(0))), Real>);
  static_assert(std::is_same_v<decltype(std::comp_ellint_2(Real(0))), Real>);
  static_assert(std::is_same_v<decltype(std::comp_ellint_3(Real(0), Real(0))), Real>);
  static_assert(std::is_same_v<decltype(std::ellint_1(Real(0), Real(0.5))), Real>);
  static_assert(std::is_same_v<decltype(std::ellint_2(Real(0), Real(0.5))), Real>);
  static_assert(std::is_same_v<decltype(std::ellint_3(Real(0), Real(0), Real(0.5))), Real>);
  static_assert(std::is_same_v<decltype(std::expint(Real(1))), Real>);
  static_assert(std::is_same_v<decltype(std::riemann_zeta(Real(2))), Real>);
  static_assert(std::is_same_v<decltype(std::cyl_bessel_j(Real(0), Real(1))), Real>);
  static_assert(std::is_same_v<decltype(std::cyl_neumann(Real(0), Real(1))), Real>);
  static_assert(std::is_same_v<decltype(std::cyl_bessel_i(Real(0), Real(1))), Real>);
  static_assert(std::is_same_v<decltype(std::cyl_bessel_k(Real(0), Real(1))), Real>);
  static_assert(std::is_same_v<decltype(std::sph_bessel(0, Real(1))), Real>);
  static_assert(std::is_same_v<decltype(std::sph_neumann(0, Real(1))), Real>);
}

int main(int, char**) {
  test<float>();
  test<double>();
  test<long double>();
  test<std::float16_t>();
  test<std::bfloat16_t>();
  test<std::float32_t>();
  test<std::float64_t>();
#if defined(__STDCPP_FLOAT128_T__)
  test<std::float128_t>();
#endif
  return 0;
}
