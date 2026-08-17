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
// floating-point-type ellint_1(Arg1 k, Arg2 phi);
// float               ellint_1f(float k, float phi);
// long double         ellint_1l(long double k, long double phi);

#include <array>
#include <cassert>
#include <cmath>
#include <limits>
#include <numbers>

#include "type_algorithms.h"

// Preconditions require -1 <= k <= 1 ([sf.cmath]).
template <class T>
std::array<T, 5> k_points() {
  return {T(0.0), T(0.3), T(0.5), T(0.7), T(0.9)};
}

template <class T>
std::array<T, 4> phi_points() {
  return {T(0.1), T(0.5), T(1.0), T(1.3)};
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
        return 1e-9;
      else
        return 1e-10l;
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
      assert(std::isnan(std::ellint_1(NaN, Real(1))));
      assert(std::isnan(std::ellint_1(Real(0.5), NaN)));
    }
  }

  { // F(0, k) == 0: exact boundary identity.
    for (Real k : k_points<Real>())
      assert(std::ellint_1(k, Real(0)) == Real(0));
  }

  { // k == 0 reduces to the identity function: F(phi, 0) == phi.
    const CompareFloatingValues<Real> compare;
    for (Real phi : phi_points<Real>())
      assert(compare(std::ellint_1(Real(0), phi), phi));
  }

  { // phi == pi/2 reduces to the complete elliptic integral of the first kind.
    const CompareFloatingValues<Real> compare;
    for (Real k : k_points<Real>())
      assert(compare(std::ellint_1(k, std::numbers::pi_v<Real> / 2), std::comp_ellint_1(k)));
  }

  { // Independent oracle spot checks for k = 0.5.
    const CompareFloatingValues<Real> compare;
    assert(compare(std::ellint_1(Real(0.5), Real(0.1)), Real(0.1000416301342917)));
    assert(compare(std::ellint_1(Real(0.5), Real(0.5)), Real(0.5050887275786482)));
    assert(compare(std::ellint_1(Real(0.5), Real(1.0)), Real(1.0373561200021773)));
    assert(compare(std::ellint_1(Real(0.5), Real(1.3)), Real(1.3743036662883921)));
  }

  { // F(phi, k) is monotonically increasing in phi for fixed k, over the sampled sub-pi/2 range.
    for (Real k : k_points<Real>()) {
      auto phis = phi_points<Real>();
      for (std::size_t i = 0; i + 1 < phis.size(); ++i)
        assert(std::ellint_1(k, phis[i]) <= std::ellint_1(k, phis[i + 1]));
    }
  }

  if constexpr (std::is_same_v<Real, float>)
    for (float k : k_points<float>())
      for (float phi : phi_points<float>())
        assert(std::ellint_1(k, phi) == std::ellint_1f(k, phi));

  if constexpr (std::is_same_v<Real, long double>)
    for (long double k : k_points<long double>())
      for (long double phi : phi_points<long double>())
        assert(std::ellint_1(k, phi) == std::ellint_1l(k, phi));
}

struct TestFloat {
  template <class Real>
  void operator()() {
    test<Real>();
  }
};

int main(int, char**) {
  types::for_each(types::floating_point_types(), TestFloat());

  assert(std::ellint_1(0, 0.5) == std::ellint_1(0.0, 0.5));
  assert(std::ellint_1(0.5f, 0.5) == std::ellint_1(0.5, 0.5));
  assert(std::ellint_1(0.5, 0.5l) == std::ellint_1(0.5l, 0.5l));

  return 0;
}
