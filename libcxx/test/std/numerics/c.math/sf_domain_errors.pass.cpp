//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14

// <cmath>

// [sf.cmath.general]: the mathematical special functions report a domain error for the argument values
// outside the domain their description specifies and where the mathematical function is not defined,
// and only for those values; an argument that is a NaN gives a NaN without a domain error.

#include <cassert>
#include <cerrno>
#include <cfenv>
#include <cmath>
#include <limits>

#include "test_macros.h"

template <class F>
bool reports_domain_error(F f) {
  errno = 0;
  std::feclearexcept(FE_ALL_EXCEPT);
  auto v = f();
  if (!std::isnan(v))
    return false;
  bool ok = true;
  if (math_errhandling & MATH_ERRNO)
    ok = ok && errno == EDOM;
  if (math_errhandling & MATH_ERREXCEPT)
    ok = ok && std::fetestexcept(FE_INVALID);
  return ok;
}

template <class F>
bool reports_nothing(F f) {
  errno = 0;
  std::feclearexcept(FE_ALL_EXCEPT);
  (void)f();
  return errno != EDOM && !std::fetestexcept(FE_INVALID);
}

template <class F>
bool is_positive_infinity(F f) {
  auto v = f();
  return std::isinf(v) && v > 0;
}

template <class Real>
void test() {
  const Real nan = std::numeric_limits<Real>::quiet_NaN();

  // Explicit domains.
  assert(reports_domain_error([] { return std::assoc_laguerre(2, 1, Real(-0.5)); }));
  assert(reports_domain_error([] { return std::laguerre(2, Real(-0.5)); }));
  assert(reports_domain_error([] { return std::assoc_legendre(2, 1, Real(1.5)); }));
  assert(reports_domain_error([] { return std::legendre(2, Real(1.5)); }));
  assert(reports_domain_error([] { return std::legendre(2, Real(-1.5)); }));
  assert(reports_domain_error([] { return std::beta(Real(-1), Real(2)); }));
  assert(reports_domain_error([] { return std::beta(Real(2), Real(0)); }));
  assert(reports_domain_error([] { return std::beta(Real(2), Real(-3)); }));
  assert(reports_domain_error([] { return std::comp_ellint_1(Real(1.5)); }));
  assert(reports_domain_error([] { return std::comp_ellint_2(Real(1.5)); }));
  assert(reports_domain_error([] { return std::comp_ellint_3(Real(1.5), Real(0.2)); }));
  assert(reports_domain_error([] { return std::ellint_1(Real(1.5), Real(0.3)); }));
  assert(reports_domain_error([] { return std::ellint_2(Real(1.5), Real(0.3)); }));
  assert(reports_domain_error([] { return std::ellint_3(Real(1.5), Real(0.2), Real(0.3)); }));
  assert(reports_domain_error([] { return std::cyl_bessel_i(Real(1), Real(-0.5)); }));
  assert(reports_domain_error([] { return std::cyl_bessel_j(Real(1), Real(-0.5)); }));
  assert(reports_domain_error([] { return std::cyl_bessel_k(Real(1), Real(-0.5)); }));
  assert(reports_domain_error([] { return std::cyl_neumann(Real(1), Real(-0.5)); }));
  assert(reports_domain_error([] { return std::sph_bessel(2, Real(-0.5)); }));
  assert(reports_domain_error([] { return std::sph_neumann(2, Real(-0.5)); }));
  assert(reports_domain_error([] { return std::sph_legendre(2, 3, Real(0.5)); }));

  // Not mathematically defined: no limit that is independent of the direction of approach.
  assert(reports_domain_error([] { return std::riemann_zeta(Real(1)); }));
  assert(reports_domain_error([] { return std::comp_ellint_3(Real(0.5), Real(2)); }));
  assert(reports_domain_error([] { return std::ellint_3(Real(0.5), Real(2), Real(1)); }));
  assert(reports_domain_error([] { return std::ellint_3(Real(0.5), Real(2), Real(2)); }));
  assert(reports_domain_error([] { return std::ellint_3(Real(0.5), Real(1.5), Real(1.5)); }));

  // The boundaries of the domains are inside them.
  assert(reports_nothing([] { return std::laguerre(2, Real(0)); }));
  assert(reports_nothing([] { return std::laguerre(2, Real(-0.0)); }));
  assert(reports_nothing([] { return std::legendre(2, Real(1)); }));
  assert(reports_nothing([] { return std::legendre(2, Real(-1)); }));
  assert(reports_nothing([] { return std::assoc_legendre(2, 1, Real(1)); }));
  assert(reports_nothing([] { return std::assoc_legendre(1, 2, Real(0.5)); })); // m > l: the value is 0
  assert(reports_nothing([] { return std::sph_legendre(2, 2, Real(0.5)); }));
  assert(reports_nothing([] { return std::sph_bessel(1, Real(0)); }));
  assert(reports_nothing([] { return std::cyl_bessel_i(Real(1), Real(0)); }));
  assert(reports_nothing([] { return std::cyl_bessel_j(Real(1), Real(0)); }));
  assert(reports_nothing([] { return std::ellint_1(Real(1), Real(0.5)); }));
  assert(reports_nothing([] { return std::ellint_2(Real(1), Real(1.5)); }));
  assert(reports_nothing([] { return std::comp_ellint_2(Real(1)); }));
  assert(reports_nothing([] { return std::ellint_3(Real(0.5), Real(2), Real(0.5)); }));
  assert(reports_nothing([] { return std::ellint_3(Real(0.5), Real(-2), Real(1)); }));
  assert(reports_nothing([] { return std::comp_ellint_3(Real(0.5), Real(0.5)); }));
  assert(reports_nothing([] { return std::comp_ellint_3(Real(0.5), Real(-3)); }));

  // The limit exists and is +infinity.
  assert(is_positive_infinity([] { return std::comp_ellint_1(Real(1)); }));
  assert(is_positive_infinity([] { return std::comp_ellint_1(Real(-1)); }));
  assert(is_positive_infinity([] { return std::comp_ellint_3(Real(0.5), Real(1)); }));
  assert(is_positive_infinity([] { return std::comp_ellint_3(Real(1), Real(0.5)); }));

  // A NaN argument gives a NaN and no domain error, even next to an argument outside the domain.
  assert(reports_nothing([=] { return std::legendre(2, nan); }));
  assert(std::isnan(std::legendre(2, nan)));
  assert(reports_nothing([=] { return std::beta(nan, Real(-1)); }));
  assert(std::isnan(std::beta(nan, Real(-1))));
  assert(reports_nothing([=] { return std::ellint_1(Real(1.5), nan); }));
}

int main(int, char**) {
  test<float>();
  test<double>();
  test<long double>();
  return 0;
}
