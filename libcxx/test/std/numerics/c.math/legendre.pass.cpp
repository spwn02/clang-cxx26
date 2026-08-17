//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14

// <cmath>

// double         legendre(unsigned l, double x);
// float          legendre(unsigned l, float x);
// long double    legendre(unsigned l, long double x);
// float          legendref(unsigned l, float x);
// long double    legendrel(unsigned l, long double x);
// template <class Integer>
// double         legendre(unsigned l, Integer x);

#include <array>
#include <cassert>
#include <cmath>
#include <limits>

#include "type_algorithms.h"

template <class T>
std::array<T, 7> sample_points() {
  return {T(-0.99), T(-0.7), T(-0.3), T(0.0), T(0.3), T(0.7), T(0.99)};
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
        return 1e-5f;
      else if (std::is_same_v<Real, double>)
        return 1e-11;
      else
        return 1e-12l;
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
      for (unsigned l = 0; l < 20; ++l)
        assert(std::isnan(std::legendre(l, NaN)));
  }

  { // std::legendre(l, x) for l = 0..5 against the closed-form Legendre polynomials.
    const auto p0 = [](Real) -> Real { return 1; };
    const auto p1 = [](Real x) -> Real { return x; };
    const auto p2 = [](Real x) -> Real { return (3 * x * x - 1) / 2; };
    const auto p3 = [](Real x) -> Real { return (5 * x * x * x - 3 * x) / 2; };
    const auto p4 = [](Real x) -> Real { return (35 * std::pow(x, 4) - 30 * x * x + 3) / 8; };
    const auto p5 = [](Real x) -> Real { return (63 * std::pow(x, 5) - 70 * std::pow(x, 3) + 15 * x) / 8; };

    const CompareFloatingValues<Real> compare;
    for (Real x : sample_points<Real>()) {
      assert(compare(std::legendre(0, x), p0(x)));
      assert(compare(std::legendre(1, x), p1(x)));
      assert(compare(std::legendre(2, x), p2(x)));
      assert(compare(std::legendre(3, x), p3(x)));
      assert(compare(std::legendre(4, x), p4(x)));
      assert(compare(std::legendre(5, x), p5(x)));
    }
  }

  { // P_l(1) == 1 and P_l(-1) == (-1)^l for every l -- exact boundary identities, not just close.
    for (unsigned l = 0; l < 30; ++l) {
      assert(std::legendre(l, Real(1)) == Real(1));
      Real expect_minus_one = (l & 1) ? Real(-1) : Real(1);
      const CompareFloatingValues<Real> compare;
      assert(compare(std::legendre(l, Real(-1)), expect_minus_one));
    }
  }

  { // Bonnet's recursion: (l+1) P_{l+1}(x) = (2l+1) x P_l(x) - l P_{l-1}(x).
    const CompareFloatingValues<Real> compare;
    for (Real x : sample_points<Real>())
      for (unsigned l = 1; l < 40; ++l) {
        Real lhs = (l + 1) * std::legendre(l + 1, x);
        Real rhs = (2 * l + 1) * x * std::legendre(l, x) - l * std::legendre(l - 1, x);
        assert(compare(lhs, rhs));
      }
  }

  { // |P_l(x)| <= 1 for |x| <= 1, for every l -- a property that would catch a runaway recursion.
    for (Real x : sample_points<Real>())
      for (unsigned l = 0; l < 40; ++l)
        assert(std::abs(std::legendre(l, x)) <= Real(1) + std::numeric_limits<Real>::epsilon() * 100);
  }

  if constexpr (std::is_same_v<Real, float>)
    for (unsigned l = 0; l < 20; ++l)
      for (float x : sample_points<float>())
        assert(std::legendre(l, x) == std::legendref(l, x));

  if constexpr (std::is_same_v<Real, long double>)
    for (unsigned l = 0; l < 20; ++l)
      for (long double x : sample_points<long double>())
        assert(std::legendre(l, x) == std::legendrel(l, x));
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
    for (unsigned l = 0; l < 20; ++l)
      for (Integer x : {-1, 0, 1})
        assert(std::legendre(l, x) == std::legendre(l, static_cast<double>(x)));
  }
};

int main(int, char**) {
  types::for_each(types::floating_point_types(), TestFloat());
  types::for_each(types::type_list<short, int, long, long long>(), TestInt());
  return 0;
}
