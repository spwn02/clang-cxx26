//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14

// <cmath>

// template <class Arg1, class Arg2, class Arg3>
// floating-point-type ellint_3(Arg1 k, Arg2 nu, Arg3 phi);
// float               ellint_3f(float k, float nu, float phi);
// long double         ellint_3l(long double k, long double nu, long double phi);

#include <array>
#include <cassert>
#include <cmath>
#include <limits>
#include <numbers>

#include "type_algorithms.h"

// Preconditions require -1 <= k <= 1, nu < 1 ([sf.cmath]).
template <class T>
std::array<T, 3> k_points() {
  return {T(0.0), T(0.5), T(0.9)};
}

template <class T>
std::array<T, 3> nu_points() {
  return {T(-0.5), T(0.2), T(0.8)};
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
      assert(std::isnan(std::ellint_3(NaN, Real(0.2), Real(1))));
      assert(std::isnan(std::ellint_3(Real(0.5), NaN, Real(1))));
      assert(std::isnan(std::ellint_3(Real(0.5), Real(0.2), NaN)));
    }
  }

  { // Pi(0, nu, k) == 0: exact boundary identity.
    for (Real k : k_points<Real>())
      for (Real nu : nu_points<Real>())
        assert(std::ellint_3(k, nu, Real(0)) == Real(0));
  }

  { // nu == 0 reduces ellint_3 to ellint_1.
    const CompareFloatingValues<Real> compare;
    for (Real k : k_points<Real>())
      for (Real phi : phi_points<Real>())
        assert(compare(std::ellint_3(k, Real(0), phi), std::ellint_1(k, phi)));
  }

  { // phi == pi/2 reduces to the complete elliptic integral of the third kind.
    const CompareFloatingValues<Real> compare;
    for (Real k : k_points<Real>())
      for (Real nu : nu_points<Real>())
        assert(compare(std::ellint_3(k, nu, std::numbers::pi_v<Real> / 2), std::comp_ellint_3(k, nu)));
  }

  { // Independent oracle spot checks for (k, nu) = (0.5, 0.2).
    const CompareFloatingValues<Real> compare;
    assert(compare(std::ellint_3(Real(0.5), Real(0.2), Real(0.1)), Real(0.10010829322815123)));
    assert(compare(std::ellint_3(Real(0.5), Real(0.2), Real(0.5)), Real(0.51339281401952497)));
    assert(compare(std::ellint_3(Real(0.5), Real(0.2), Real(1.0)), Real(1.1013048164778225)));
    assert(compare(std::ellint_3(Real(0.5), Real(0.2), Real(1.3)), Real(1.5052985806837262)));
  }

  if constexpr (std::is_same_v<Real, float>)
    for (float k : k_points<float>())
      for (float nu : nu_points<float>())
        for (float phi : phi_points<float>())
          assert(std::ellint_3(k, nu, phi) == std::ellint_3f(k, nu, phi));

  if constexpr (std::is_same_v<Real, long double>)
    for (long double k : k_points<long double>())
      for (long double nu : nu_points<long double>())
        for (long double phi : phi_points<long double>())
          assert(std::ellint_3(k, nu, phi) == std::ellint_3l(k, nu, phi));
}

struct TestFloat {
  template <class Real>
  void operator()() {
    test<Real>();
  }
};

int main(int, char**) {
  types::for_each(types::floating_point_types(), TestFloat());

  // Every literal below (0, 0.25, 0.5) is exactly representable in binary floating point at every
  // width, so promoting to a wider type is lossless and the results are bit-identical (unlike, say,
  // 0.2, whose double and long double roundings differ, which would make an exact `==` flaky here).
  assert(std::ellint_3(0, 0.25, 0.5) == std::ellint_3(0.0, 0.25, 0.5));
  assert(std::ellint_3(0.5f, 0.25, 0.5) == std::ellint_3(0.5, 0.25, 0.5));
  assert(std::ellint_3(0.5, 0.25, 0.5l) == std::ellint_3(0.5l, 0.25l, 0.5l));

  return 0;
}
