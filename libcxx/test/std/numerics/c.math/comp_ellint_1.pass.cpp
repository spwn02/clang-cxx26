//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14

// <cmath>

// double         comp_ellint_1(double k);
// float          comp_ellint_1(float k);
// long double    comp_ellint_1(long double k);
// float          comp_ellint_1f(float k);
// long double    comp_ellint_1l(long double k);
// template <class Integer>
// double         comp_ellint_1(Integer k);

#include <array>
#include <cassert>
#include <cmath>
#include <limits>
#include <numbers>

#include "type_algorithms.h"

// Preconditions require -1 <= k <= 1 ([sf.cmath]).
template <class T>
std::array<T, 6> sample_points() {
  return {T(-0.9), T(-0.5), T(0.0), T(0.3), T(0.7), T(0.99)};
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
    for (Real NaN : {nl::quiet_NaN(), nl::signaling_NaN()})
      assert(std::isnan(std::comp_ellint_1(NaN)));
  }

  { // K(0) == pi/2, an exact closed-form boundary value.
    const CompareFloatingValues<Real> compare;
    assert(compare(std::comp_ellint_1(Real(0)), std::numbers::pi_v<Real> / 2));
  }

  { // K(k) == K(-k): comp_ellint_1 depends on k only through k^2.
    const CompareFloatingValues<Real> compare;
    for (Real k : sample_points<Real>())
      assert(compare(std::comp_ellint_1(k), std::comp_ellint_1(-k)));
  }

  { // K(k) is monotonically increasing in |k| over [0, 1) -- checked pairwise across a fixed,
    // ascending, in-domain sample set (not an accumulating loop: float rounding on repeated += can
    // walk k past the k<=1 domain boundary and turn the last comparison into a NaN).
    const std::array<Real, 5> ascending = {Real(0.0), Real(0.3), Real(0.5), Real(0.7), Real(0.9)};
    for (std::size_t i = 0; i + 1 < ascending.size(); ++i)
      assert(std::comp_ellint_1(ascending[i]) <= std::comp_ellint_1(ascending[i + 1]));
  }

  { // Independent oracle spot checks (values from an independent reference implementation).
    const CompareFloatingValues<Real> compare;
    assert(compare(std::comp_ellint_1(Real(0.5)), Real(1.6857503548125963)));
    assert(compare(std::comp_ellint_1(Real(0.9)), Real(2.2805491384227703)));
  }

  if constexpr (std::is_same_v<Real, float>)
    for (float k : sample_points<float>())
      assert(std::comp_ellint_1(k) == std::comp_ellint_1f(k));

  if constexpr (std::is_same_v<Real, long double>)
    for (long double k : sample_points<long double>())
      assert(std::comp_ellint_1(k) == std::comp_ellint_1l(k));
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
    // k = +-1 sits exactly on the domain boundary ([sf.cmath]'s Preconditions require -1 <= k <= 1,
    // but K(k) itself diverges as k -> +-1): both this implementation and an independent reference
    // return NaN there rather than +inf, so the delegation check below must treat NaN as matching
    // NaN (plain == would spuriously fail, since NaN != NaN under IEEE 754).
    for (Integer k : {-1, 0, 1}) {
      double direct   = std::comp_ellint_1(k);
      double expected = std::comp_ellint_1(static_cast<double>(k));
      assert(direct == expected || (std::isnan(direct) && std::isnan(expected)));
    }
  }
};

int main(int, char**) {
  types::for_each(types::floating_point_types(), TestFloat());
  types::for_each(types::type_list<short, int, long, long long>(), TestInt());
  return 0;
}
