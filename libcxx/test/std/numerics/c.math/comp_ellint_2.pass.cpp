//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14

// <cmath>

// double         comp_ellint_2(double k);
// float          comp_ellint_2(float k);
// long double    comp_ellint_2(long double k);
// float          comp_ellint_2f(float k);
// long double    comp_ellint_2l(long double k);
// template <class Integer>
// double         comp_ellint_2(Integer k);

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
      assert(std::isnan(std::comp_ellint_2(NaN)));
  }

  { // E(0) == pi/2, E(1) == 1: exact closed-form boundary values.
    const CompareFloatingValues<Real> compare;
    assert(compare(std::comp_ellint_2(Real(0)), std::numbers::pi_v<Real> / 2));
    assert(std::comp_ellint_2(Real(1)) == Real(1));
    assert(std::comp_ellint_2(Real(-1)) == Real(1));
  }

  { // E(k) == E(-k): comp_ellint_2 depends on k only through k^2.
    const CompareFloatingValues<Real> compare;
    for (Real k : sample_points<Real>())
      assert(compare(std::comp_ellint_2(k), std::comp_ellint_2(-k)));
  }

  { // E(k) is monotonically decreasing in |k| over [0, 1] -- checked pairwise across a fixed,
    // ascending, in-domain sample set (see comp_ellint_1's test for why not an accumulating loop).
    const std::array<Real, 5> ascending = {Real(0.0), Real(0.3), Real(0.5), Real(0.7), Real(0.9)};
    for (std::size_t i = 0; i + 1 < ascending.size(); ++i)
      assert(std::comp_ellint_2(ascending[i]) >= std::comp_ellint_2(ascending[i + 1]));
  }

  { // Independent oracle spot checks.
    const CompareFloatingValues<Real> compare;
    assert(compare(std::comp_ellint_2(Real(0.5)), Real(1.4674622093394274)));
    assert(compare(std::comp_ellint_2(Real(0.9)), Real(1.1716970527816053)));
  }

  if constexpr (std::is_same_v<Real, float>)
    for (float k : sample_points<float>())
      assert(std::comp_ellint_2(k) == std::comp_ellint_2f(k));

  if constexpr (std::is_same_v<Real, long double>)
    for (long double k : sample_points<long double>())
      assert(std::comp_ellint_2(k) == std::comp_ellint_2l(k));
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
    for (Integer k : {-1, 0, 1})
      assert(std::comp_ellint_2(k) == std::comp_ellint_2(static_cast<double>(k)));
  }
};

int main(int, char**) {
  types::for_each(types::floating_point_types(), TestFloat());
  types::for_each(types::type_list<short, int, long, long long>(), TestInt());
  return 0;
}
