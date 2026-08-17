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
// floating-point-type comp_ellint_3(Arg1 k, Arg2 nu);
// float               comp_ellint_3f(float k, float nu);
// long double         comp_ellint_3l(long double k, long double nu);

#include <array>
#include <cassert>
#include <cmath>
#include <limits>

#include "type_algorithms.h"

// Preconditions require -1 <= k <= 1, nu < 1 ([sf.cmath]).
template <class T>
std::array<T, 5> k_points() {
  return {T(0.0), T(0.3), T(0.5), T(0.7), T(0.9)};
}

template <class T>
std::array<T, 5> nu_points() {
  return {T(-0.5), T(0.0), T(0.2), T(0.5), T(0.8)};
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
      assert(std::isnan(std::comp_ellint_3(NaN, Real(0.2))));
      assert(std::isnan(std::comp_ellint_3(Real(0.5), NaN)));
    }
  }

  { // nu == 0 reduces comp_ellint_3 to comp_ellint_1.
    const CompareFloatingValues<Real> compare;
    for (Real k : k_points<Real>())
      assert(compare(std::comp_ellint_3(k, Real(0)), std::comp_ellint_1(k)));
  }

  { // Independent oracle spot checks for (k, nu) = (0.5, {-0.5, 0.2, 0.5, 0.8}).
    const CompareFloatingValues<Real> compare;
    assert(compare(std::comp_ellint_3(Real(0.5), Real(-0.5)), Real(1.3664739530045971)));
    assert(compare(std::comp_ellint_3(Real(0.5), Real(0.2)), Real(1.8922947612264023)));
    assert(compare(std::comp_ellint_3(Real(0.5), Real(0.5)), Real(2.4136715042011949)));
    assert(compare(std::comp_ellint_3(Real(0.5), Real(0.8)), Real(3.8750701888108074)));
  }

  { // Pi(k, nu) is monotonically increasing in nu for fixed k, over the sampled sub-1 range.
    for (Real k : k_points<Real>()) {
      auto nus = nu_points<Real>();
      for (std::size_t i = 0; i + 1 < nus.size(); ++i)
        assert(std::comp_ellint_3(k, nus[i]) <= std::comp_ellint_3(k, nus[i + 1]));
    }
  }

  if constexpr (std::is_same_v<Real, float>)
    for (float k : k_points<float>())
      for (float nu : nu_points<float>())
        assert(std::comp_ellint_3(k, nu) == std::comp_ellint_3f(k, nu));

  if constexpr (std::is_same_v<Real, long double>)
    for (long double k : k_points<long double>())
      for (long double nu : nu_points<long double>())
        assert(std::comp_ellint_3(k, nu) == std::comp_ellint_3l(k, nu));
}

struct TestFloat {
  template <class Real>
  void operator()() {
    test<Real>();
  }
};

int main(int, char**) {
  types::for_each(types::floating_point_types(), TestFloat());

  // Mixed-argument overload resolves via the standard's common-floating-point-type promotion.
  assert(std::comp_ellint_3(0, 0.2) == std::comp_ellint_3(0.0, 0.2));
  assert(std::comp_ellint_3(0.5f, 0.2) == std::comp_ellint_3(0.5, 0.2));
  assert(std::comp_ellint_3(0.5, 0.2l) == std::comp_ellint_3(0.5l, 0.2l));

  return 0;
}
