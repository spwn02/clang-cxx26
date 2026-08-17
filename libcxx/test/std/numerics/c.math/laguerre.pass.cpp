//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14

// <cmath>

// double         laguerre(unsigned n, double x);
// float          laguerre(unsigned n, float x);
// long double    laguerre(unsigned n, long double x);
// float          laguerref(unsigned n, float x);
// long double    laguerrel(unsigned n, long double x);
// template <class Integer>
// double         laguerre(unsigned n, Integer x);

#include <array>
#include <cassert>
#include <cmath>
#include <limits>

#include "type_algorithms.h"

// laguerre's Preconditions require x >= 0 ([sf.cmath]) -- unlike legendre/assoc_legendre, whose
// domain is symmetric, every sample point here stays non-negative.
template <class T>
std::array<T, 6> sample_points() {
  return {T(0.0), T(0.5), T(1.0), T(2.3), T(5.0), T(10.0)};
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
    if (std::isinf(expected) && std::isinf(result))
      return result == expected;
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
      for (unsigned n = 0; n < 20; ++n)
        assert(std::isnan(std::laguerre(n, NaN)));
  }

  { // std::laguerre(n, x) for n = 0..4 against the closed-form Laguerre polynomials.
    const auto l0 = [](Real) -> Real { return 1; };
    const auto l1 = [](Real x) -> Real { return 1 - x; };
    const auto l2 = [](Real x) -> Real { return 1 - 2 * x + x * x / 2; };
    const auto l3 = [](Real x) -> Real { return 1 - 3 * x + Real(1.5) * x * x - x * x * x / 6; };
    const auto l4 = [](Real x) -> Real {
      return 1 - 4 * x + 3 * x * x - (Real(2) / 3) * x * x * x + std::pow(x, 4) / 24;
    };

    const CompareFloatingValues<Real> compare;
    for (Real x : sample_points<Real>()) {
      assert(compare(std::laguerre(0, x), l0(x)));
      assert(compare(std::laguerre(1, x), l1(x)));
      assert(compare(std::laguerre(2, x), l2(x)));
      assert(compare(std::laguerre(3, x), l3(x)));
      assert(compare(std::laguerre(4, x), l4(x)));
    }
  }

  { // L_n(0) == 1 for every n -- an exact boundary identity from the closed form's constant term.
    for (unsigned n = 0; n < 30; ++n)
      assert(std::laguerre(n, Real(0)) == Real(1));
  }

  { // Recursion: (k+1) L_{k+1}(x) = (2k+1-x) L_k(x) - k L_{k-1}(x).
    const CompareFloatingValues<Real> compare;
    for (Real x : sample_points<Real>())
      for (unsigned k = 1; k < 25; ++k) {
        Real lhs = (k + 1) * std::laguerre(k + 1, x);
        Real rhs = (2 * k + 1 - x) * std::laguerre(k, x) - k * std::laguerre(k - 1, x);
        if (std::isinf(lhs))
          break;
        assert(compare(lhs, rhs));
      }
  }

  if constexpr (std::numeric_limits<Real>::has_infinity) {
    // Overflow for large x -> infinity, signed by the parity of n (leading term (-x)^n / n! for
    // x >= 0), matching the sign convention documented in special_functions.h.
    if constexpr (std::is_same_v<Real, double>) {
      double inf = std::numeric_limits<double>::infinity();
      for (unsigned n = 100; n < 130; ++n) {
        double v = std::laguerre(n, 1e6);
        if (std::isinf(v))
          assert(v == ((n & 1) ? -inf : inf));
      }
    }
  }

  if constexpr (std::is_same_v<Real, float>)
    for (unsigned n = 0; n < 20; ++n)
      for (float x : sample_points<float>())
        assert(std::laguerre(n, x) == std::laguerref(n, x));

  if constexpr (std::is_same_v<Real, long double>)
    for (unsigned n = 0; n < 20; ++n)
      for (long double x : sample_points<long double>())
        assert(std::laguerre(n, x) == std::laguerrel(n, x));
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
    for (unsigned n = 0; n < 20; ++n)
      for (Integer x : {0, 1, 5, 7, 42})
        assert(std::laguerre(n, x) == std::laguerre(n, static_cast<double>(x)));
  }
};

int main(int, char**) {
  types::for_each(types::floating_point_types(), TestFloat());
  types::for_each(types::type_list<short, int, long, long long>(), TestInt());
  return 0;
}
