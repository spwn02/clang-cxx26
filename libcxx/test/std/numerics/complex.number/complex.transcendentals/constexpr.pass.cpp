//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <complex>

// P1383R2 ("More constexpr for <cmath> and <complex>"): sqrt, exp, log, log10, pow, sinh, cosh,
// tanh, asinh, acosh, atanh, sin, cos, tan, asin, acos, atan, abs, arg, polar, and proj all become
// usable in a constant expression.
//
// This is a dedicated smoke test rather than an extension of each function's existing
// complex.transcendentals/<name>.pass.cpp file: those files' test_edges() loops iterate over a
// shared NaN/infinity-heavy testcases[] array, and this fork's compiler has a pre-existing,
// unrelated constexpr step-limit issue evaluating that same style of loop for other complex
// operations (confirmed independently of this change against complex.ops/complex_divide_complex and
// complex.ops/complex_times_complex, both of which already fail identically with an unmodified
// <complex>). Keeping the constexpr checks here, over a small fixed set of ordinary values, verifies
// the new capability without depending on that separate, pre-existing limitation being fixed first.

#include <cassert>
#include <complex>
#include <limits>

#include "test_macros.h"

template <class T>
constexpr bool test() {
  using C = std::complex<T>;

  // abs, arg, polar, proj
  {
    static_assert(std::is_same<decltype(std::abs(C(3, 4))), T>::value, "");
    assert(std::abs(C(3, 4)) == T(5));
    assert(std::arg(C(0, 1)) > T(1.57) && std::arg(C(0, 1)) < T(1.58));
    C p = std::polar(T(1), T(0));
    assert(p.real() > T(0.99) && p.real() < T(1.01));
    C pr = std::proj(C(std::numeric_limits<T>::infinity(), -T(2)));
    assert(std::isinf(pr.real()));
    assert(std::signbit(pr.imag()));
  }

  // sqrt, exp, log, log10
  {
    C s = std::sqrt(C(4, 0));
    assert(s.real() > T(1.99) && s.real() < T(2.01));
    C e = std::exp(C(0, 0));
    assert(e.real() > T(0.99) && e.real() < T(1.01));
    C l = std::log(C(1, 0));
    assert(l.real() > -T(0.01) && l.real() < T(0.01));
    C l10 = std::log10(C(100, 0));
    assert(l10.real() > T(1.99) && l10.real() < T(2.01));
  }

  // pow (all three overloads)
  {
    C p1 = std::pow(C(2, 0), C(3, 0));
    assert(p1.real() > T(7.9) && p1.real() < T(8.1));
    C p2 = std::pow(C(2, 0), T(3));
    assert(p2.real() > T(7.9) && p2.real() < T(8.1));
    C p3 = std::pow(T(2), C(3, 0));
    assert(p3.real() > T(7.9) && p3.real() < T(8.1));
  }

  // sinh, cosh, tanh, asinh, acosh, atanh
  {
    C sh = std::sinh(C(0, 0));
    assert(sh.real() == T(0) && sh.imag() == T(0));
    C ch = std::cosh(C(0, 0));
    assert(ch.real() > T(0.99) && ch.real() < T(1.01));
    C th = std::tanh(C(0, 0));
    assert(th.real() == T(0) && th.imag() == T(0));
    C ash = std::asinh(C(0, 0));
    assert(ash.real() == T(0) && ash.imag() == T(0));
    C ach = std::acosh(C(1, 0));
    assert(ach.real() > -T(0.01) && ach.real() < T(0.01));
    C ath = std::atanh(C(0, 0));
    assert(ath.real() == T(0) && ath.imag() == T(0));
  }

  // sin, cos, tan, asin, acos, atan
  {
    C sn = std::sin(C(0, 0));
    assert(sn.real() == T(0) && sn.imag() == T(0));
    C cn = std::cos(C(0, 0));
    assert(cn.real() > T(0.99) && cn.real() < T(1.01));
    C tn = std::tan(C(0, 0));
    assert(tn.real() == T(0) && tn.imag() == T(0));
    C asn = std::asin(C(0, 0));
    assert(asn.real() == T(0) && asn.imag() == T(0));
    C acn = std::acos(C(1, 0));
    assert(acn.real() > -T(0.01) && acn.real() < T(0.01));
    C atn = std::atan(C(0, 0));
    assert(atn.real() == T(0) && atn.imag() == T(0));
  }

  // A round trip through several functions at once, at a non-trivial value, cross-checked against
  // the corresponding identity rather than a literal (so it's meaningful evidence beyond "compiles").
  // The tolerance is loose enough to hold even for float (~7 significant digits).
  {
    C z(0.7, -1.3);
    T tol = T(1e-4);
    C round_trip = std::log(std::exp(z));
    // log(exp(z)) == z only up to a multiple of 2*pi*i in general, but for this z (imaginary part
    // magnitude well under pi) it should recover z directly.
    assert(std::abs(round_trip - z) < tol);

    C sinsq_cossq = std::sin(z) * std::sin(z) + std::cos(z) * std::cos(z);
    assert(std::abs(sinsq_cossq - C(1, 0)) < tol);
  }

  return true;
}

int main(int, char**) {
  static_assert(test<float>(), "");
  static_assert(test<double>(), "");
  static_assert(test<long double>(), "");

  assert(test<float>());
  assert(test<double>());
  assert(test<long double>());

  return 0;
}
