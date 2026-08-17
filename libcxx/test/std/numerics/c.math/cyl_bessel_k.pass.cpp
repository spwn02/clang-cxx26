//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14

// <cmath>

// template <class Arg1, class Arg2>
// floating-point-type cyl_bessel_k(Arg1 nu, Arg2 x);
// float               cyl_bessel_kf(float nu, float x);
// long double         cyl_bessel_kl(long double nu, long double x);

#include <array>
#include <cassert>
#include <cmath>
#include <limits>

#include "type_algorithms.h"

// Preconditions require nu >= 0, x > 0 ([sf.cmath]: K_nu has a singularity at x == 0). x deliberately
// spans both sides of the internal cancellation-avoiding threshold used by the implementation (see
// __cyl_bessel_k's comment in __math/special_functions.h), so this test exercises both code paths.
template <class T>
std::array<T, 6> nu_points() {
  return {T(0.0), T(1.0), T(2.0), T(0.5), T(1.5), T(5.5)};
}

template <class T>
std::array<T, 5> x_points() {
  return {T(0.5), T(1.0), T(2.0), T(5.0), T(15.0)};
}

template <class Real>
class CompareFloatingValues {
private:
  Real abs_tol;
  Real rel_tol;

public:
  CompareFloatingValues() {
    abs_tol = []() -> Real {
      if (std::is_same_v<Real, float>)
        return 1e-3f;
      else if (std::is_same_v<Real, double>)
        return 1e-4;
      else
        return 1e-5l;
    }();
    rel_tol = abs_tol;
  }

  bool operator()(Real result, Real expected) const {
    if (std::isnan(expected) || std::isnan(result))
      return false;
    Real tol = abs_tol + std::abs(expected) * rel_tol;
    return std::abs(result - expected) < tol;
  }
};

template <class Real>
void test() {
  if constexpr (std::numeric_limits<Real>::has_quiet_NaN && std::numeric_limits<Real>::has_signaling_NaN) {
    using nl = std::numeric_limits<Real>;
    for (Real NaN : {nl::quiet_NaN(), nl::signaling_NaN()}) {
      assert(std::isnan(std::cyl_bessel_k(NaN, Real(1))));
      assert(std::isnan(std::cyl_bessel_k(Real(0.5), NaN)));
    }
  }

  { // K_nu(x) > 0 everywhere on the domain.
    for (Real nu : nu_points<Real>())
      for (Real x : x_points<Real>())
        assert(std::cyl_bessel_k(nu, x) > 0);
  }

  { // Independent oracle spot checks (values from an independent reference implementation).
    const CompareFloatingValues<Real> compare;
    assert(compare(std::cyl_bessel_k(Real(0), Real(1)), Real(0.42102443824070834)));
    assert(compare(std::cyl_bessel_k(Real(1), Real(2)), Real(0.13986588181652262)));
    assert(compare(std::cyl_bessel_k(Real(0.5), Real(5)), Real(0.003776613374642883)));
  }

  { // Recurrence: K_{nu-1}(x) - K_{nu+1}(x) == -(2 nu / x) K_nu(x). Skips nu == 0: nu - 1 == -1
    // there, and negative integer order is outside this function's documented domain (nu >= 0).
    const CompareFloatingValues<Real> compare;
    for (Real nu : nu_points<Real>()) {
      if (nu < 1)
        continue;
      for (Real x : x_points<Real>()) {
        Real lhs = std::cyl_bessel_k(nu - 1, x) - std::cyl_bessel_k(nu + 1, x);
        Real rhs = -(2 * nu / x) * std::cyl_bessel_k(nu, x);
        assert(compare(lhs, rhs));
      }
    }
  }

  { // Wronskian: I_nu(x) K_{nu+1}(x) + I_{nu+1}(x) K_nu(x) == 1/x. Ties cyl_bessel_i and
    // cyl_bessel_k together at a fixed normalization, catching a normalization error that a
    // same-function-family recurrence check alone would miss.
    const CompareFloatingValues<Real> compare;
    for (Real nu : nu_points<Real>())
      for (Real x : x_points<Real>()) {
        Real lhs = std::cyl_bessel_i(nu, x) * std::cyl_bessel_k(nu + 1, x) +
                   std::cyl_bessel_i(nu + 1, x) * std::cyl_bessel_k(nu, x);
        assert(compare(lhs, 1 / x));
      }
  }

  if constexpr (std::is_same_v<Real, float>)
    for (float nu : nu_points<float>())
      for (float x : x_points<float>())
        assert(std::cyl_bessel_k(nu, x) == std::cyl_bessel_kf(nu, x));

  if constexpr (std::is_same_v<Real, long double>)
    for (long double nu : nu_points<long double>())
      for (long double x : x_points<long double>())
        assert(std::cyl_bessel_k(nu, x) == std::cyl_bessel_kl(nu, x));
}

struct TestFloat {
  template <class Real>
  void operator()() {
    test<Real>();
  }
};

int main(int, char**) {
  types::for_each(types::floating_point_types(), TestFloat());

  assert(std::cyl_bessel_k(0, 1.0) == std::cyl_bessel_k(0.0, 1.0));
  assert(std::cyl_bessel_k(1.0f, 2.0) == std::cyl_bessel_k(1.0, 2.0));
  assert(std::cyl_bessel_k(1.0, 2.0l) == std::cyl_bessel_k(1.0l, 2.0l));

  return 0;
}
