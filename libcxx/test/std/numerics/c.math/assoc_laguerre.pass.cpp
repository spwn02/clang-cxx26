//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14

// <cmath>

// double         assoc_laguerre(unsigned n, unsigned m, double x);
// float          assoc_laguerre(unsigned n, unsigned m, float x);
// long double    assoc_laguerre(unsigned n, unsigned m, long double x);
// float          assoc_laguerref(unsigned n, unsigned m, float x);
// long double    assoc_laguerrel(unsigned n, unsigned m, long double x);
// template <class Integer>
// double         assoc_laguerre(unsigned n, unsigned m, Integer x);

#include <array>
#include <cassert>
#include <cmath>
#include <limits>

#include "type_algorithms.h"

// Preconditions require x >= 0 ([sf.cmath]), same as laguerre.
template <class T>
std::array<T, 5> sample_points() {
  return {T(0.0), T(0.5), T(1.0), T(2.3), T(5.0)};
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
      for (unsigned n = 0; n < 10; ++n)
        assert(std::isnan(std::assoc_laguerre(n, 1, NaN)));
  }

  { // m == 0 reduces to the ordinary Laguerre polynomial for every n.
    const CompareFloatingValues<Real> compare;
    for (Real x : sample_points<Real>())
      for (unsigned n = 0; n < 15; ++n)
        assert(compare(std::assoc_laguerre(n, 0, x), std::laguerre(n, x)));
  }

  { // Closed forms: L_n^{(m)}(x) = sum_i (-1)^i C(n+m, n-i) x^i / i!, for n = 0..3.
    const CompareFloatingValues<Real> compare;
    for (Real x : sample_points<Real>())
      for (unsigned m = 0; m <= 4; ++m) {
        Real dm = static_cast<Real>(m);
        Real l0 = 1;
        Real l1 = -x + dm + 1;
        Real l2 = x * x / 2 - (dm + 2) * x + (dm + 1) * (dm + 2) / 2;
        Real l3 = -x * x * x / 6 + (dm + 3) * x * x / 2 - (dm + 2) * (dm + 3) * x / 2 +
                  (dm + 1) * (dm + 2) * (dm + 3) / 6;
        assert(compare(std::assoc_laguerre(0, m, x), l0));
        assert(compare(std::assoc_laguerre(1, m, x), l1));
        assert(compare(std::assoc_laguerre(2, m, x), l2));
        assert(compare(std::assoc_laguerre(3, m, x), l3));
      }
  }

  { // L_n^{(m)}(0) == C(n+m, n) -- an exact boundary identity, checked via the recurrence's own
    // arithmetic (binomial coefficient built up incrementally) rather than a separate factorial
    // formula that could hide a matching mistake.
    for (unsigned m = 0; m <= 4; ++m) {
      Real binom = 1;
      for (unsigned n = 0; n < 15; ++n) {
        const CompareFloatingValues<Real> compare;
        assert(compare(std::assoc_laguerre(n, m, Real(0)), binom));
        binom = binom * static_cast<Real>(n + m + 1) / static_cast<Real>(n + 1);
      }
    }
  }

  { // Recursion: (k+1) L_{k+1}^m(x) = (2k+1+m-x) L_k^m(x) - (k+m) L_{k-1}^m(x).
    const CompareFloatingValues<Real> compare;
    for (Real x : sample_points<Real>())
      for (unsigned m = 0; m <= 3; ++m)
        for (unsigned k = 1; k < 20; ++k) {
          Real lhs = (k + 1) * std::assoc_laguerre(k + 1, m, x);
          Real rhs = (2 * k + 1 + m - x) * std::assoc_laguerre(k, m, x) - (k + m) * std::assoc_laguerre(k - 1, m, x);
          if (std::isinf(lhs))
            break;
          assert(compare(lhs, rhs));
        }
  }

  if constexpr (std::is_same_v<Real, float>)
    for (unsigned n = 0; n < 15; ++n)
      for (unsigned m = 0; m <= 3; ++m)
        for (float x : sample_points<float>())
          assert(std::assoc_laguerre(n, m, x) == std::assoc_laguerref(n, m, x));

  if constexpr (std::is_same_v<Real, long double>)
    for (unsigned n = 0; n < 15; ++n)
      for (unsigned m = 0; m <= 3; ++m)
        for (long double x : sample_points<long double>())
          assert(std::assoc_laguerre(n, m, x) == std::assoc_laguerrel(n, m, x));
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
    for (unsigned n = 0; n < 15; ++n)
      for (unsigned m = 0; m <= 3; ++m)
        for (Integer x : {0, 1, 5, 7, 42})
          assert(std::assoc_laguerre(n, m, x) == std::assoc_laguerre(n, m, static_cast<double>(x)));
  }
};

int main(int, char**) {
  types::for_each(types::floating_point_types(), TestFloat());
  types::for_each(types::type_list<short, int, long, long long>(), TestInt());
  return 0;
}
