//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14

// <cmath>

// floating_point riemann_zeta(arithmetic x);
// float          riemann_zetaf(float x);
// long double    riemann_zetal(long double x);

#include <array>
#include <cassert>
#include <cmath>
#include <limits>

#include "type_algorithms.h"

// Preconditions require x != 1 ([sf.cmath]); x == 1 is exercised separately below.
template <class T>
std::array<T, 8> sample_points() {
  return {T(-5.0), T(-3.0), T(-1.0), T(-0.5), T(0.0), T(0.5), T(1.5), T(2.0)};
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
      assert(std::isnan(std::riemann_zeta(NaN)));
  }

  { // zeta(1) is a pole: +infinity.
    if constexpr (std::numeric_limits<Real>::has_infinity)
      assert(std::isinf(std::riemann_zeta(Real(1))) && std::riemann_zeta(Real(1)) > 0);
  }

  { // Trivial zeros: zeta(s) == 0 exactly at every negative even integer.
    for (Real s : {Real(-2), Real(-4), Real(-6), Real(-8)})
      assert(std::riemann_zeta(s) == Real(0));
  }

  { // zeta(0) == -1/2, zeta(-1) == -1/12: exact closed-form boundary values (the latter is the
    // famous "Ramanujan summation" value, a genuine identity of the analytically continued
    // function, not the divergent series' naive sum).
    const CompareFloatingValues<Real> compare;
    assert(compare(std::riemann_zeta(Real(0)), Real(-0.5)));
    assert(compare(std::riemann_zeta(Real(-1)), Real(-1) / 12));
  }

  { // Independent oracle spot checks (values from an independent reference implementation).
    const CompareFloatingValues<Real> compare;
    assert(compare(std::riemann_zeta(Real(-5)), Real(-0.0039682539682539663)));
    assert(compare(std::riemann_zeta(Real(-3)), Real(0.0083333333333333332)));
    assert(compare(std::riemann_zeta(Real(-0.5)), Real(-0.20788622497735459)));
    assert(compare(std::riemann_zeta(Real(0.5)), Real(-1.4603545088095922)));
    assert(compare(std::riemann_zeta(Real(1.5)), Real(2.6123753486854886)));
    assert(compare(std::riemann_zeta(Real(2)), Real(1.6449340668482264)));
    assert(compare(std::riemann_zeta(Real(10)), Real(1.0009945751278182)));
  }

  { // zeta(s) -> 1 as s -> +infinity: checked via a monotonically-decreasing-toward-1 tail.
    assert(std::riemann_zeta(Real(20)) > Real(1));
    assert(std::riemann_zeta(Real(20)) < std::riemann_zeta(Real(10)));
  }

  if constexpr (std::is_same_v<Real, float>)
    for (float s : sample_points<float>())
      assert(std::riemann_zeta(s) == std::riemann_zetaf(s));

  if constexpr (std::is_same_v<Real, long double>)
    for (long double s : sample_points<long double>())
      assert(std::riemann_zeta(s) == std::riemann_zetal(s));
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
    for (Integer s : {-5, -3, 0, 2, 10})
      assert(std::riemann_zeta(s) == std::riemann_zeta(static_cast<double>(s)));
  }
};

int main(int, char**) {
  types::for_each(types::floating_point_types(), TestFloat());
  types::for_each(types::type_list<short, int, long, long long>(), TestInt());
  return 0;
}
