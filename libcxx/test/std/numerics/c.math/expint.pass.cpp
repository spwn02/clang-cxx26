//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14

// <cmath>

// floating_point expint(arithmetic x);
// float          expintf(float x);
// long double    expintl(long double x);

#include <array>
#include <cassert>
#include <cmath>
#include <limits>

#include "type_algorithms.h"

// Preconditions require x != 0 ([sf.cmath]); x == 0 is exercised separately below.
template <class T>
std::array<T, 8> sample_points() {
  return {T(-5.0), T(-1.0), T(-0.1), T(0.1), T(0.5), T(1.0), T(5.0), T(20.0)};
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
        return 1e-4f;
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
    for (Real NaN : {nl::quiet_NaN(), nl::signaling_NaN()})
      assert(std::isnan(std::expint(NaN)));
  }

  { // expint(0) is negative infinity: Ei(x) has a logarithmic singularity as x -> 0 from either side.
    if constexpr (std::numeric_limits<Real>::has_infinity) {
      Real v = std::expint(Real(0));
      assert(std::isinf(v) && v < 0);
    }
  }

  { // Independent oracle spot checks (values from an independent reference implementation).
    const CompareFloatingValues<Real> compare;
    assert(compare(std::expint(Real(-10)), Real(-4.1569689296853263e-06)));
    assert(compare(std::expint(Real(-1)), Real(-0.21938393439551984)));
    assert(compare(std::expint(Real(-0.5)), Real(-0.55977359477616073)));
    assert(compare(std::expint(Real(0.5)), Real(0.45421990486317354)));
    assert(compare(std::expint(Real(1)), Real(1.895117816355937)));
    assert(compare(std::expint(Real(2)), Real(4.9542343560018907)));
    assert(compare(std::expint(Real(5)), Real(40.185275355803171)));
    assert(compare(std::expint(Real(10)), Real(2492.2289762418764)));
  }

  { // Ei'(x) = e^x/x, which is negative for x < 0 and positive for x > 0: Ei is strictly decreasing
    // on the domain's negative half and strictly increasing on its positive half (not monotonic
    // across the whole domain -- e.g. Ei(-5) ~ -0.00115 but Ei(-0.1) ~ -1.82, more negative despite
    // -0.1 > -5). The step is scaled to the sample's own magnitude rather than fixed at 0.01: at
    // x = 20, Ei(x) ~ 2.5e7, and float's ~7 significant digits can't distinguish Ei(20) from
    // Ei(20.01) -- that would be a precision artifact of this check, not a real violation.
    for (Real x : sample_points<Real>()) {
      Real step = std::abs(x) < Real(1) ? Real(0.01) : Real(0.01) * std::abs(x);
      if (x < 0)
        assert(std::expint(x) > std::expint(x + step));
      else
        assert(std::expint(x) < std::expint(x + step));
    }
  }

  { // Ei(x) < 0 for every x < 0: E1(z) = integral from z to infinity of e^-t/t dt is a positive-kernel
    // integral for every z > 0, so Ei(x) = -E1(-x) is negative throughout x < 0. (Ei(x) for x > 0 has
    // no comparable universal sign -- it has a real root near x ~ 0.3725, so e.g. Ei(0.1) < 0 while
    // Ei(0.5) > 0; that's why this check is one-sided.)
    for (Real x : sample_points<Real>())
      if (x < 0)
        assert(std::expint(x) < 0);
  }

  if constexpr (std::is_same_v<Real, float>)
    for (float x : sample_points<float>())
      assert(std::expint(x) == std::expintf(x));

  if constexpr (std::is_same_v<Real, long double>)
    for (long double x : sample_points<long double>())
      assert(std::expint(x) == std::expintl(x));
}

struct TestFloat {
  template <class Real>
  void operator()() {
    test<Real>();
  }
};

struct TestInt {
  template <class Integer>
  void operator()() {
    for (Integer x : {-5, -1, 1, 5, 20})
      assert(std::expint(x) == std::expint(static_cast<double>(x)));
  }
};

int main(int, char**) {
  types::for_each(types::floating_point_types(), TestFloat());
  types::for_each(types::type_list<short, int, long, long long>(), TestInt());
  return 0;
}
