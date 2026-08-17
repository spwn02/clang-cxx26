//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14

// <cmath>

// double         assoc_legendre(unsigned l, unsigned m, double x);
// float          assoc_legendre(unsigned l, unsigned m, float x);
// long double    assoc_legendre(unsigned l, unsigned m, long double x);
// float          assoc_legendref(unsigned l, unsigned m, float x);
// long double    assoc_legendrel(unsigned l, unsigned m, long double x);
// template <class Integer>
// double         assoc_legendre(unsigned l, unsigned m, Integer x);

#include <array>
#include <cassert>
#include <cmath>
#include <limits>

#include "type_algorithms.h"

template <class T>
std::array<T, 5> sample_points() {
  return {T(-0.8), T(-0.3), T(0.0), T(0.4), T(0.9)};
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
        return 1e-10;
      else
        return 1e-11l;
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
      for (unsigned l = 0; l < 10; ++l)
        assert(std::isnan(std::assoc_legendre(l, 1, NaN)));
  }

  { // m == 0 reduces to the ordinary Legendre polynomial for every l.
    const CompareFloatingValues<Real> compare;
    for (Real x : sample_points<Real>())
      for (unsigned l = 0; l < 15; ++l)
        assert(compare(std::assoc_legendre(l, 0, x), std::legendre(l, x)));
  }

  { // Closed forms with the Condon-Shortley phase, per [sf.cmath]'s definition of P_l^m.
    const CompareFloatingValues<Real> compare;
    for (Real x : sample_points<Real>()) {
      Real s = std::sqrt(Real(1) - x * x);
      assert(compare(std::assoc_legendre(1, 1, x), -s));
      assert(compare(std::assoc_legendre(2, 1, x), -3 * x * s));
      assert(compare(std::assoc_legendre(2, 2, x), 3 * (1 - x * x)));
      assert(compare(std::assoc_legendre(3, 1, x), Real(-1.5) * (5 * x * x - 1) * s));
      assert(compare(std::assoc_legendre(3, 2, x), 15 * x * (1 - x * x)));
      assert(compare(std::assoc_legendre(3, 3, x), -15 * std::pow(1 - x * x, Real(1.5))));
    }
  }

  { // l == m: P_m^m(x) must alternate sign with m at a fixed x inside (0, 1) (from the (-1)^m factor
    // in the closed form) -- exercised at a fixed x for m = 0..10, distinct from the individual
    // closed-form spot checks above.
    Real x = Real(0.5);
    Real prev_abs = std::abs(std::assoc_legendre(0, 0, x));
    for (unsigned m = 1; m <= 10; ++m) {
      Real v = std::assoc_legendre(m, m, x);
      bool expect_negative = (m & 1) != 0;
      assert((v < 0) == expect_negative);
      assert(std::abs(v) != prev_abs); // strictly changes magnitude, not a stuck recursion
      prev_abs = std::abs(v);
    }
  }

  if constexpr (std::is_same_v<Real, float>)
    for (unsigned l = 0; l < 10; ++l)
      for (unsigned m = 0; m <= l; ++m)
        for (float x : sample_points<float>())
          assert(std::assoc_legendre(l, m, x) == std::assoc_legendref(l, m, x));

  if constexpr (std::is_same_v<Real, long double>)
    for (unsigned l = 0; l < 10; ++l)
      for (unsigned m = 0; m <= l; ++m)
        for (long double x : sample_points<long double>())
          assert(std::assoc_legendre(l, m, x) == std::assoc_legendrel(l, m, x));
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
    for (unsigned l = 0; l < 10; ++l)
      for (unsigned m = 0; m <= l; ++m)
        for (Integer x : {-1, 0, 1})
          assert(std::assoc_legendre(l, m, x) == std::assoc_legendre(l, m, static_cast<double>(x)));
  }
};

int main(int, char**) {
  types::for_each(types::floating_point_types(), TestFloat());
  types::for_each(types::type_list<short, int, long, long long>(), TestInt());
  return 0;
}
