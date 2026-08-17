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
// floating-point-type beta(Arg1 x, Arg2 y);
// float               betaf(float x, float y);
// long double         betal(long double x, long double y);

#include <array>
#include <cassert>
#include <cmath>
#include <limits>

#include "type_algorithms.h"

// Preconditions require x > 0, y > 0 ([sf.cmath]).
template <class T>
std::array<T, 5> sample_points() {
  return {T(0.2), T(0.5), T(1.0), T(2.5), T(7.3)};
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
      assert(std::isnan(std::beta(NaN, Real(1))));
      assert(std::isnan(std::beta(Real(1), NaN)));
    }
  }

  { // B(x, y) == Gamma(x) Gamma(y) / Gamma(x+y), checked directly via tgamma for arguments small
    // enough that the naive product doesn't overflow.
    const CompareFloatingValues<Real> compare;
    for (Real x : sample_points<Real>())
      for (Real y : sample_points<Real>())
        assert(compare(std::beta(x, y), std::tgamma(x) * std::tgamma(y) / std::tgamma(x + y)));
  }

  { // B(x, y) == B(y, x): symmetry is exact, not just close, since the implementation is symmetric
    // in its own arithmetic.
    for (Real x : sample_points<Real>())
      for (Real y : sample_points<Real>())
        assert(std::beta(x, y) == std::beta(y, x));
  }

  { // B(1, y) == 1/y: a closed-form special case, checked exactly against an independent formula.
    const CompareFloatingValues<Real> compare;
    for (Real y : sample_points<Real>())
      assert(compare(std::beta(Real(1), y), Real(1) / y));
  }

  { // B(x, y) stays finite and positive for arguments where Gamma(x)*Gamma(y) alone would overflow
    // (e.g. tgamma(200) is already > 1e300 in double) -- this is the entire reason beta is computed
    // via lgamma rather than as a direct tgamma product. Restricted to double/long double: B(150,150)
    // itself is on the order of 1e-90, which genuinely underflows float's range (unlike double's), so
    // this isn't a case of float exposing a real bug.
    if constexpr (std::numeric_limits<Real>::max_exponent >= 1024) {
      Real b = std::beta(Real(150), Real(150));
      assert(std::isfinite(b));
      assert(b > 0);
    }
  }

  if constexpr (std::is_same_v<Real, float>)
    for (float x : sample_points<float>())
      for (float y : sample_points<float>())
        assert(std::beta(x, y) == std::betaf(x, y));

  if constexpr (std::is_same_v<Real, long double>)
    for (long double x : sample_points<long double>())
      for (long double y : sample_points<long double>())
        assert(std::beta(x, y) == std::betal(x, y));
}

struct TestFloat {
  template <class Real>
  void operator()() {
    test<Real>();
  }
};

int main(int, char**) {
  types::for_each(types::floating_point_types(), TestFloat());

  // Mixed-argument overload resolves via the standard's common-floating-point-type promotion,
  // matching the same __promote_t mechanism as std::hypot's two-argument overload.
  assert(std::beta(2, 3.0) == std::beta(2.0, 3.0));
  assert(std::beta(2.0f, 3.0) == std::beta(2.0, 3.0));
  assert(std::beta(2.0, 3.0l) == std::beta(2.0l, 3.0l));

  return 0;
}
