//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <simd>

// [simd.math] and [simd.complex.math] declare same-named function templates in the same namespace
// (simd::abs, simd::exp, simd::sqrt, simd::pow, ...): one set operating on a real-valued
// simd-floating-point V, the other on a simd-complex V whose value_type is a complex<T>
// specialization. This file exercises that every shared name resolves to the *correct* one of the
// two for both a real vec and a complex vec, and that std::simd's plain-namespace-std re-exports
// (in <simd> itself) carry both overloads together without introducing ambiguity against each
// other or against the pre-existing scalar std:: overloads for the same names.
//
// __simd_complex and __math_floating_point are disjoint by construction (the former requires
// value_type::value_type to exist and match complex<T>; the latter requires value_type to satisfy
// floating_point directly, which a complex<T> specialization never does) -- so this "shouldn't" be
// able to go wrong. It is tested anyway because permute.h and loadstore.h each had a real,
// shipped ambiguity bug of exactly this shape earlier in this port (an unconstrained static-permute
// overload matching dynamic-permute calls; an internal helper with the same signature as its public
// overload) before the fix. A same-shaped ambiguity bug in math.h/complex.h would look identical:
// SFINAE succeeding on both instead of exactly one.

#include <cassert>
#include <cmath>
#include <complex>
#include <concepts>
#include <simd>

int main(int, char**) {
  using RV = std::simd::vec<double, 4>;
  using CV = std::simd::vec<std::complex<double>, 4>;

  RV re([](auto i) { return static_cast<double>(i) + 1.0; });
  RV im([](auto i) { return static_cast<double>(i) * 0.5; });
  CV c(re, im);

  // simd::exp: real-vec overload must return RV (math.h), complex-vec overload must return CV
  // (complex.h) -- same unqualified name, disjoint constraints, no ambiguity.
  {
    auto r1 = std::simd::exp(re);
    auto r2 = std::simd::exp(c);
    static_assert(std::same_as<decltype(r1), RV>);
    static_assert(std::same_as<decltype(r2), CV>);
    assert(r1[0] == std::exp(re[0]));
    assert(r2[0] == std::exp(std::complex<double>(re[0], im[0])));
  }

  // simd::abs has three logically distinct overloads sharing one name: signed-integral vec
  // (math.h), floating-point vec (math.h), and complex vec -> real-type vec (complex.h). Exercise
  // the floating and complex ones here (the integral one is covered by basic_vec.pass.cpp).
  {
    auto r1 = std::simd::abs(re);
    auto r2 = std::simd::abs(c);
    static_assert(std::same_as<decltype(r1), RV>);
    static_assert(std::same_as<decltype(r2), RV>); // complex abs returns the real-type vec, not CV
    assert(r1[0] == std::abs(re[0]));
    assert(r2[0] == std::abs(std::complex<double>(re[0], im[0])));
  }

  // simd::sqrt / simd::pow: same disambiguation, plus pow is 2-argument so both operands must
  // agree on real-vs-complex for either overload to be viable at all.
  {
    auto s1 = std::simd::sqrt(re);
    auto s2 = std::simd::sqrt(c);
    static_assert(std::same_as<decltype(s1), RV>);
    static_assert(std::same_as<decltype(s2), CV>);
    assert(s1[0] == std::sqrt(re[0]));
    assert(s2[0] == std::sqrt(std::complex<double>(re[0], im[0])));

    auto p1 = std::simd::pow(re, re);
    auto p2 = std::simd::pow(c, c);
    static_assert(std::same_as<decltype(p1), RV>);
    static_assert(std::same_as<decltype(p2), CV>);
  }

  // Plain std:: re-exports (declared directly in <simd>, not std::simd): the same disambiguation
  // must hold when called unqualified through std::, and must not collide with the pre-existing
  // scalar std::exp/std::abs/std::sqrt/std::pow overloads for plain double / std::complex<double>.
  {
    auto r1 = std::exp(re);
    auto r2 = std::exp(c);
    double r3 = std::exp(1.0);
    std::complex<double> r4 = std::exp(std::complex<double>(1.0, 2.0));
    static_assert(std::same_as<decltype(r1), RV>);
    static_assert(std::same_as<decltype(r2), CV>);
    assert(r1[0] == std::simd::exp(re)[0]);
    assert(r2[0] == std::simd::exp(c)[0]);
    assert(r3 > 2.7 && r3 < 2.8);
    assert(r4 == std::exp(std::complex<double>(1.0, 2.0)));
  }
  {
    double a1 = std::abs(-3.0);
    std::complex<double> sc(3.0, 4.0);
    double a2  = std::abs(sc); // pre-existing scalar std::abs(complex<T>)
    auto a3 = std::abs(re);
    auto a4 = std::abs(c);
    assert(a1 == 3.0);
    assert(a2 == 5.0);
    static_assert(std::same_as<decltype(a3), RV>);
    static_assert(std::same_as<decltype(a4), RV>);
  }

  // simd::real / simd::imag / simd::conj / simd::arg / simd::norm / simd::proj / simd::polar are
  // unique to complex.h (no math.h counterpart), but real/imag/conj/arg/norm/proj also collide by
  // name with the pre-existing *scalar* std::real(complex<T>) etc. from <complex> once re-exported
  // into plain std::. Check the vec and scalar forms both resolve.
  {
    auto r1 = std::real(c);
    static_assert(std::same_as<decltype(r1), RV>);
    assert(r1[0] == re[0]);

    std::complex<double> sc(3.0, 4.0);
    double r2 = std::real(sc);
    assert(r2 == 3.0);
  }

  return 0;
}
