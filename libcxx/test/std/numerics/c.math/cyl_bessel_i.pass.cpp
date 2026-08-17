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
// floating-point-type cyl_bessel_i(Arg1 nu, Arg2 x);
// float               cyl_bessel_if(float nu, float x);
// long double         cyl_bessel_il(long double nu, long double x);

#include <array>
#include <cassert>
#include <cmath>
#include <limits>

#include "type_algorithms.h"

// Preconditions require nu >= 0, x >= 0 ([sf.cmath]).
template <class T>
std::array<T, 5> nu_points() {
  return {T(0.0), T(1.0), T(2.0), T(0.5), T(1.5)};
}

template <class T>
std::array<T, 4> x_points() {
  return {T(0.5), T(1.0), T(2.0), T(5.0)};
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
        return 1e-6;
      else
        return 1e-7l;
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
      assert(std::isnan(std::cyl_bessel_i(NaN, Real(1))));
      assert(std::isnan(std::cyl_bessel_i(Real(0.5), NaN)));
    }
  }

  { // I_0(0) == 1, I_nu(0) == 0 for nu > 0: exact boundary identities from the series' leading term.
    assert(std::cyl_bessel_i(Real(0), Real(0)) == Real(1));
    for (Real nu : {Real(1), Real(2), Real(0.5), Real(1.5)})
      assert(std::cyl_bessel_i(nu, Real(0)) == Real(0));
  }

  { // I_nu(x) > 0 everywhere on the domain (unlike J_nu, I_nu has no zeros for x > 0, nu >= 0).
    for (Real nu : nu_points<Real>())
      for (Real x : x_points<Real>())
        assert(std::cyl_bessel_i(nu, x) > 0);
  }

  { // Independent oracle spot checks (values from an independent reference implementation).
    const CompareFloatingValues<Real> compare;
    assert(compare(std::cyl_bessel_i(Real(0), Real(1)), Real(1.2660658777520082)));
    assert(compare(std::cyl_bessel_i(Real(1), Real(2)), Real(1.5906368546373288)));
    assert(compare(std::cyl_bessel_i(Real(0.5), Real(5)), Real(26.477547497559065)));
  }

  { // Recurrence: I_{nu-1}(x) - I_{nu+1}(x) == (2 nu / x) I_nu(x). Skips nu == 0: nu - 1 == -1 there,
    // and negative integer order is outside this function's documented domain (nu >= 0).
    const CompareFloatingValues<Real> compare;
    for (Real nu : nu_points<Real>()) {
      if (nu < 1)
        continue;
      for (Real x : x_points<Real>()) {
        Real lhs = std::cyl_bessel_i(nu - 1, x) - std::cyl_bessel_i(nu + 1, x);
        Real rhs = (2 * nu / x) * std::cyl_bessel_i(nu, x);
        assert(compare(lhs, rhs));
      }
    }
  }

  if constexpr (std::is_same_v<Real, float>)
    for (float nu : nu_points<float>())
      for (float x : x_points<float>())
        assert(std::cyl_bessel_i(nu, x) == std::cyl_bessel_if(nu, x));

  if constexpr (std::is_same_v<Real, long double>)
    for (long double nu : nu_points<long double>())
      for (long double x : x_points<long double>())
        assert(std::cyl_bessel_i(nu, x) == std::cyl_bessel_il(nu, x));
}

struct TestFloat {
  template <class Real>
  void operator()() {
    test<Real>();
  }
};

int main(int, char**) {
  types::for_each(types::floating_point_types(), TestFloat());

  assert(std::cyl_bessel_i(0, 1.0) == std::cyl_bessel_i(0.0, 1.0));
  assert(std::cyl_bessel_i(1.0f, 2.0) == std::cyl_bessel_i(1.0, 2.0));
  assert(std::cyl_bessel_i(1.0, 2.0l) == std::cyl_bessel_i(1.0l, 2.0l));

  return 0;
}
