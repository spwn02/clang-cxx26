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
// floating-point-type cyl_neumann(Arg1 nu, Arg2 x);
// float               cyl_neumannf(float nu, float x);
// long double         cyl_neumannl(long double nu, long double x);

#include <array>
#include <cassert>
#include <cmath>
#include <limits>
#include <numbers>

#include "type_algorithms.h"

// Preconditions require nu >= 0, x > 0 ([sf.cmath]: Y_nu has a singularity at x == 0).
template <class T>
std::array<T, 5> nu_points() {
  return {T(0.0), T(1.0), T(2.0), T(0.5), T(1.5)};
}

template <class T>
std::array<T, 5> x_points() {
  return {T(0.5), T(1.0), T(2.0), T(5.0), T(10.0)};
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
        return 1e-5;
      else
        return 1e-6l;
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
      assert(std::isnan(std::cyl_neumann(NaN, Real(1))));
      assert(std::isnan(std::cyl_neumann(Real(0.5), NaN)));
    }
  }

  { // Independent oracle spot checks (values from an independent reference implementation).
    const CompareFloatingValues<Real> compare;
    assert(compare(std::cyl_neumann(Real(0), Real(1)), Real(0.088256964215676942)));
    assert(compare(std::cyl_neumann(Real(1), Real(2)), Real(-0.10703243154093725)));
    assert(compare(std::cyl_neumann(Real(0.5), Real(5)), Real(-0.10121770918510853)));
  }

  { // Recurrence: Y_{nu-1}(x) + Y_{nu+1}(x) == (2 nu / x) Y_nu(x). Skips nu == 0: nu - 1 == -1 there,
    // and negative order is outside this function's documented domain (nu >= 0).
    const CompareFloatingValues<Real> compare;
    for (Real nu : nu_points<Real>()) {
      if (nu < 1)
        continue;
      for (Real x : x_points<Real>()) {
        Real lhs = std::cyl_neumann(nu - 1, x) + std::cyl_neumann(nu + 1, x);
        Real rhs = (2 * nu / x) * std::cyl_neumann(nu, x);
        assert(compare(lhs, rhs));
      }
    }
  }

  { // Wronskian: J_nu(x) Y_{nu-1}(x) - J_{nu-1}(x) Y_nu(x) == 2/(pi x). Ties cyl_bessel_j and
    // cyl_neumann together at a fixed normalization, catching a normalization error that a
    // same-function-family recurrence check alone would miss. Same nu >= 1 restriction as above.
    const CompareFloatingValues<Real> compare;
    for (Real nu : nu_points<Real>()) {
      if (nu < 1)
        continue;
      for (Real x : x_points<Real>()) {
        Real lhs = std::cyl_bessel_j(nu, x) * std::cyl_neumann(nu - 1, x) -
                   std::cyl_bessel_j(nu - 1, x) * std::cyl_neumann(nu, x);
        Real rhs = 2 / (std::numbers::pi_v<Real> * x);
        assert(compare(lhs, rhs));
      }
    }
  }

  if constexpr (std::is_same_v<Real, float>)
    for (float nu : nu_points<float>())
      for (float x : x_points<float>())
        assert(std::cyl_neumann(nu, x) == std::cyl_neumannf(nu, x));

  if constexpr (std::is_same_v<Real, long double>)
    for (long double nu : nu_points<long double>())
      for (long double x : x_points<long double>())
        assert(std::cyl_neumann(nu, x) == std::cyl_neumannl(nu, x));
}

struct TestFloat {
  template <class Real>
  void operator()() {
    test<Real>();
  }
};

int main(int, char**) {
  types::for_each(types::floating_point_types(), TestFloat());

  assert(std::cyl_neumann(0, 1.0) == std::cyl_neumann(0.0, 1.0));
  assert(std::cyl_neumann(1.0f, 2.0) == std::cyl_neumann(1.0, 2.0));
  assert(std::cyl_neumann(1.0, 2.0l) == std::cyl_neumann(1.0l, 2.0l));

  return 0;
}
