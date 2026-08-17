//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14

// <cmath>

// double         sph_neumann(unsigned n, double x);
// float          sph_neumann(unsigned n, float x);
// long double    sph_neumann(unsigned n, long double x);
// float          sph_neumannf(unsigned n, float x);
// long double    sph_neumannl(unsigned n, long double x);
// template <class Integer>
// double         sph_neumann(unsigned n, Integer x);

#include <array>
#include <cassert>
#include <cmath>
#include <limits>

#include "type_algorithms.h"

// Preconditions require x > 0 ([sf.cmath]: y_n has a singularity at x == 0).
template <class T>
std::array<T, 5> x_points() {
  return {T(0.5), T(1.0), T(2.0), T(5.0), T(10.0)};
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
        return 1e-8;
      else
        return 1e-9l;
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
      for (unsigned n = 0; n < 5; ++n)
        assert(std::isnan(std::sph_neumann(n, NaN)));
  }

  { // y_0(x) == -cos(x)/x, y_1(x) == -cos(x)/x^2 - sin(x)/x: exact closed forms.
    const CompareFloatingValues<Real> compare;
    for (Real x : x_points<Real>()) {
      assert(compare(std::sph_neumann(0u, x), -std::cos(x) / x));
      assert(compare(std::sph_neumann(1u, x), -std::cos(x) / (x * x) - std::sin(x) / x));
    }
  }

  { // Independent oracle spot checks (values from an independent reference implementation).
    const CompareFloatingValues<Real> compare;
    assert(compare(std::sph_neumann(0u, Real(1)), Real(-0.54030230586813999)));
    assert(compare(std::sph_neumann(1u, Real(2)), Real(-0.35061200427605527)));
    assert(compare(std::sph_neumann(3u, Real(5)), Real(-0.015442909912994299)));
  }

  { // Recurrence: y_{n-1}(x) + y_{n+1}(x) == ((2n+1)/x) y_n(x).
    const CompareFloatingValues<Real> compare;
    for (Real x : x_points<Real>())
      for (unsigned n = 1; n < 15; ++n) {
        Real lhs = std::sph_neumann(n - 1, x) + std::sph_neumann(n + 1, x);
        Real rhs = (static_cast<Real>(2 * n + 1) / x) * std::sph_neumann(n, x);
        assert(compare(lhs, rhs));
      }
  }

  { // Wronskian: j_n(x) y_{n-1}(x) - j_{n-1}(x) y_n(x) == 1/x^2, tying sph_bessel and sph_neumann
    // together at a fixed normalization.
    const CompareFloatingValues<Real> compare;
    for (Real x : x_points<Real>())
      for (unsigned n = 1; n < 10; ++n) {
        Real lhs = std::sph_bessel(n, x) * std::sph_neumann(n - 1, x) -
                   std::sph_bessel(n - 1, x) * std::sph_neumann(n, x);
        assert(compare(lhs, 1 / (x * x)));
      }
  }

  if constexpr (std::is_same_v<Real, float>)
    for (unsigned n = 0; n < 10; ++n)
      for (float x : x_points<float>())
        assert(std::sph_neumann(n, x) == std::sph_neumannf(n, x));

  if constexpr (std::is_same_v<Real, long double>)
    for (unsigned n = 0; n < 10; ++n)
      for (long double x : x_points<long double>())
        assert(std::sph_neumann(n, x) == std::sph_neumannl(n, x));
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
    for (unsigned n = 0; n < 10; ++n)
      for (Integer x : {1, 2, 5})
        assert(std::sph_neumann(n, x) == std::sph_neumann(n, static_cast<double>(x)));
  }
};

int main(int, char**) {
  types::for_each(types::floating_point_types(), TestFloat());
  types::for_each(types::type_list<short, int, long, long long>(), TestInt());
  return 0;
}
