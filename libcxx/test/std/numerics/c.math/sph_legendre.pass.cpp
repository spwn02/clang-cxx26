//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14

// <cmath>

// double         sph_legendre(unsigned l, unsigned m, double theta);
// float          sph_legendre(unsigned l, unsigned m, float theta);
// long double    sph_legendre(unsigned l, unsigned m, long double theta);
// float          sph_legendref(unsigned l, unsigned m, float theta);
// long double    sph_legendrel(unsigned l, unsigned m, long double theta);
// template <class Integer>
// double         sph_legendre(unsigned l, unsigned m, Integer theta);

#include <array>
#include <cassert>
#include <cmath>
#include <limits>
#include <numbers>

#include "type_algorithms.h"

template <class T>
std::array<T, 5> sample_points() {
  return {T(0.1), T(0.7), T(1.0), T(1.9), T(3.0)};
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
      for (unsigned l = 0; l < 8; ++l)
        assert(std::isnan(std::sph_legendre(l, 0u, NaN)));
  }

  { // m == 0 reduces to sqrt((2l+1)/(4*pi)) * legendre(l, cos theta).
    const CompareFloatingValues<Real> compare;
    for (Real theta : sample_points<Real>())
      for (unsigned l = 0; l < 15; ++l) {
        Real expected =
            std::sqrt((2 * static_cast<Real>(l) + 1) / (4 * std::numbers::pi_v<Real>)) *
            std::legendre(l, std::cos(theta));
        assert(compare(std::sph_legendre(l, 0u, theta), expected));
      }
  }

  { // Closed form for l=2, m=1: Y_2^1(theta) = -sqrt(15/(8*pi)) * sin(theta) * cos(theta).
    const CompareFloatingValues<Real> compare;
    for (Real theta : sample_points<Real>()) {
      Real expected = -std::sqrt(Real(15) / (8 * std::numbers::pi_v<Real>)) * std::sin(theta) * std::cos(theta);
      assert(compare(std::sph_legendre(2u, 1u, theta), expected));
    }
  }

  { // Closed form for l=1, m=1: Y_1^1(theta) = -sqrt(3/(8*pi)) * sin(theta).
    const CompareFloatingValues<Real> compare;
    for (Real theta : sample_points<Real>()) {
      Real expected = -std::sqrt(Real(3) / (8 * std::numbers::pi_v<Real>)) * std::sin(theta);
      assert(compare(std::sph_legendre(1u, 1u, theta), expected));
    }
  }

  { // Orthonormality-adjacent sign check: Y_l^m alternates sign with (-1)^m at a fixed interior
    // theta as m increases across l == m (the diagonal, where the closed-form P_m^m dominates).
    for (unsigned m = 0; m <= 6; ++m) {
      Real v = std::sph_legendre(m, m, Real(1));
      if (m & 1)
        assert(v < 0);
      else
        assert(v >= 0);
    }
  }

  if constexpr (std::is_same_v<Real, float>)
    for (unsigned l = 0; l < 10; ++l)
      for (unsigned m = 0; m <= l; ++m)
        for (float theta : sample_points<float>())
          assert(std::sph_legendre(l, m, theta) == std::sph_legendref(l, m, theta));

  if constexpr (std::is_same_v<Real, long double>)
    for (unsigned l = 0; l < 10; ++l)
      for (unsigned m = 0; m <= l; ++m)
        for (long double theta : sample_points<long double>())
          assert(std::sph_legendre(l, m, theta) == std::sph_legendrel(l, m, theta));
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
        for (Integer theta : {0, 1, 2})
          assert(std::sph_legendre(l, m, theta) == std::sph_legendre(l, m, static_cast<double>(theta)));
  }
};

int main(int, char**) {
  types::for_each(types::floating_point_types(), TestFloat());
  types::for_each(types::type_list<short, int, long, long long>(), TestInt());
  return 0;
}
