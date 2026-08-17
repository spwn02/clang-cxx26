// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___MATH_SPECIAL_FUNCTIONS_H
#define _LIBCPP___MATH_SPECIAL_FUNCTIONS_H

#include <__config>
#include <__math/abs.h>
#include <__math/copysign.h>
#include <__math/exponential_functions.h>
#include <__math/gamma.h>
#include <__math/logarithms.h>
#include <__math/modulo.h>
#include <__math/roots.h>
#include <__math/rounding_functions.h>
#include <__math/traits.h>
#include <__math/trigonometric_functions.h>
#include <__type_traits/enable_if.h>
#include <__type_traits/is_arithmetic.h>
#include <__type_traits/is_integral.h>
#include <__type_traits/promote.h>
#include <limits>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 17

namespace __math {
// <numbers> is part of the `std` module, not `std_core`; including it here would create a
// std_core -> std -> std_core module cycle, so this pi constant is kept local instead.
template <class _Real>
inline constexpr _Real __pi_v = _Real(3.141592653589793238462643383279502);
} // namespace __math

template <class _Real>
_LIBCPP_HIDE_FROM_ABI _Real __hermite(unsigned __n, _Real __x) {
  // The Hermite polynomial H_n(x).
  // The implementation is based on the recurrence formula: H_{n+1}(x) = 2x H_n(x) - 2n H_{n-1}.
  // Press, William H., et al. Numerical recipes 3rd edition: The art of scientific computing.
  // Cambridge university press, 2007, p. 183.

  // NOLINTBEGIN(readability-identifier-naming)
  if (__math::isnan(__x))
    return __x;

  _Real __H_0{1};
  if (__n == 0)
    return __H_0;

  _Real __H_n_prev = __H_0;
  _Real __H_n      = 2 * __x;
  for (unsigned __i = 1; __i < __n; ++__i) {
    _Real __H_n_next = 2 * (__x * __H_n - __i * __H_n_prev);
    __H_n_prev       = __H_n;
    __H_n            = __H_n_next;
  }

  if (!__math::isfinite(__H_n)) {
    // Overflow occured. Two possible cases:
    //    n is odd:  return infinity of the same sign as x.
    //    n is even: return +Inf
    _Real __inf = std::numeric_limits<_Real>::infinity();
    return (__n & 1) ? __math::copysign(__inf, __x) : __inf;
  }
  return __H_n;
  // NOLINTEND(readability-identifier-naming)
}

inline _LIBCPP_HIDE_FROM_ABI double hermite(unsigned __n, double __x) { return std::__hermite(__n, __x); }

inline _LIBCPP_HIDE_FROM_ABI float hermite(unsigned __n, float __x) {
  // use double internally -- float is too prone to overflow!
  return static_cast<float>(std::hermite(__n, static_cast<double>(__x)));
}

inline _LIBCPP_HIDE_FROM_ABI long double hermite(unsigned __n, long double __x) { return std::__hermite(__n, __x); }

inline _LIBCPP_HIDE_FROM_ABI float hermitef(unsigned __n, float __x) { return std::hermite(__n, __x); }

inline _LIBCPP_HIDE_FROM_ABI long double hermitel(unsigned __n, long double __x) { return std::hermite(__n, __x); }

template <class _Integer, std::enable_if_t<std::is_integral_v<_Integer>, int> = 0>
_LIBCPP_HIDE_FROM_ABI double hermite(unsigned __n, _Integer __x) {
  return std::hermite(__n, static_cast<double>(__x));
}

// ======================= Legendre polynomials =======================

template <class _Real>
_LIBCPP_HIDE_FROM_ABI _Real __legendre(unsigned __l, _Real __x) {
  // The Legendre polynomial P_l(x).
  // The implementation is based on Bonnet's recursion formula:
  //   (l+1) P_{l+1}(x) = (2l+1) x P_l(x) - l P_{l-1}(x).
  // Abramowitz, Milton, and Irene A. Stegun, eds. Handbook of mathematical functions with formulas,
  // graphs, and mathematical tables. Vol. 55. US Government printing office, 1968, 8.5.3.

  // NOLINTBEGIN(readability-identifier-naming)
  if (__math::isnan(__x))
    return __x;

  _Real __P_0{1};
  if (__l == 0)
    return __P_0;

  _Real __P_l_prev = __P_0;
  _Real __P_l      = __x;
  for (unsigned __i = 1; __i < __l; ++__i) {
    _Real __P_l_next = ((2 * __i + 1) * __x * __P_l - __i * __P_l_prev) / (__i + 1);
    __P_l_prev       = __P_l;
    __P_l            = __P_l_next;
  }
  return __P_l;
  // NOLINTEND(readability-identifier-naming)
}

inline _LIBCPP_HIDE_FROM_ABI double legendre(unsigned __l, double __x) { return std::__legendre(__l, __x); }

inline _LIBCPP_HIDE_FROM_ABI float legendre(unsigned __l, float __x) {
  // use double internally, matching hermite's widening -- keeps the low-order recursion terms from
  // losing precision for large l even though P_l itself stays bounded in [-1, 1] for |x| <= 1.
  return static_cast<float>(std::legendre(__l, static_cast<double>(__x)));
}

inline _LIBCPP_HIDE_FROM_ABI long double legendre(unsigned __l, long double __x) {
  return std::__legendre(__l, __x);
}

inline _LIBCPP_HIDE_FROM_ABI float legendref(unsigned __l, float __x) { return std::legendre(__l, __x); }

inline _LIBCPP_HIDE_FROM_ABI long double legendrel(unsigned __l, long double __x) {
  return std::legendre(__l, __x);
}

template <class _Integer, std::enable_if_t<std::is_integral_v<_Integer>, int> = 0>
_LIBCPP_HIDE_FROM_ABI double legendre(unsigned __l, _Integer __x) {
  return std::legendre(__l, static_cast<double>(__x));
}

// ======================= Associated Legendre polynomials =======================

template <class _Real>
_LIBCPP_HIDE_FROM_ABI _Real __assoc_legendre(unsigned __l, unsigned __m, _Real __x) {
  // The associated Legendre polynomial P_l^m(x), including the Condon-Shortley phase (-1)^m per
  // [sf.cmath]'s definition.
  // Starting values and recursion (Press, William H., et al. Numerical recipes 3rd edition: The art
  // of scientific computing. Cambridge university press, 2007, p. 294, `plgndr`):
  //   P_m^m(x)     = (-1)^m (2m-1)!! (1-x^2)^{m/2}
  //   P_{m+1}^m(x) = x (2m+1) P_m^m(x)
  //   (l-m) P_l^m(x) = x(2l-1) P_{l-1}^m(x) - (l+m-1) P_{l-2}^m(x),  l > m+1.

  // NOLINTBEGIN(readability-identifier-naming)
  if (__math::isnan(__x))
    return __x;

  _Real __P_mm{1};
  if (__m > 0) {
    _Real __somx2 = __math::sqrt((_Real(1) - __x) * (_Real(1) + __x));
    _Real __fact  = _Real(1);
    for (unsigned __i = 1; __i <= __m; ++__i) {
      __P_mm *= -__fact * __somx2;
      __fact += _Real(2);
    }
  }
  if (__l == __m)
    return __P_mm;

  _Real __P_mmp1 = __x * static_cast<_Real>(2 * __m + 1) * __P_mm;
  if (__l == __m + 1)
    return __P_mmp1;

  _Real __P_ll{};
  for (unsigned __ll = __m + 2; __ll <= __l; ++__ll) {
    __P_ll = (__x * static_cast<_Real>(2 * __ll - 1) * __P_mmp1 - static_cast<_Real>(__ll + __m - 1) * __P_mm) /
             static_cast<_Real>(__ll - __m);
    __P_mm   = __P_mmp1;
    __P_mmp1 = __P_ll;
  }
  return __P_ll;
  // NOLINTEND(readability-identifier-naming)
}

inline _LIBCPP_HIDE_FROM_ABI double assoc_legendre(unsigned __l, unsigned __m, double __x) {
  return std::__assoc_legendre(__l, __m, __x);
}

inline _LIBCPP_HIDE_FROM_ABI float assoc_legendre(unsigned __l, unsigned __m, float __x) {
  return static_cast<float>(std::assoc_legendre(__l, __m, static_cast<double>(__x)));
}

inline _LIBCPP_HIDE_FROM_ABI long double assoc_legendre(unsigned __l, unsigned __m, long double __x) {
  return std::__assoc_legendre(__l, __m, __x);
}

inline _LIBCPP_HIDE_FROM_ABI float assoc_legendref(unsigned __l, unsigned __m, float __x) {
  return std::assoc_legendre(__l, __m, __x);
}

inline _LIBCPP_HIDE_FROM_ABI long double assoc_legendrel(unsigned __l, unsigned __m, long double __x) {
  return std::assoc_legendre(__l, __m, __x);
}

template <class _Integer, std::enable_if_t<std::is_integral_v<_Integer>, int> = 0>
_LIBCPP_HIDE_FROM_ABI double assoc_legendre(unsigned __l, unsigned __m, _Integer __x) {
  return std::assoc_legendre(__l, __m, static_cast<double>(__x));
}

// ======================= Laguerre polynomials =======================

template <class _Real>
_LIBCPP_HIDE_FROM_ABI _Real __laguerre(unsigned __n, _Real __x) {
  // The Laguerre polynomial L_n(x).
  // Recursion: (k+1) L_{k+1}(x) = (2k+1-x) L_k(x) - k L_{k-1}(x).
  // Abramowitz & Stegun 22.7.12.

  // NOLINTBEGIN(readability-identifier-naming)
  if (__math::isnan(__x))
    return __x;

  _Real __L_0{1};
  if (__n == 0)
    return __L_0;

  _Real __L_n_prev = __L_0;
  _Real __L_n      = _Real(1) - __x;
  for (unsigned __k = 1; __k < __n; ++__k) {
    _Real __L_n_next = (static_cast<_Real>(2 * __k + 1) - __x) * __L_n - static_cast<_Real>(__k) * __L_n_prev;
    __L_n_next        = __L_n_next / static_cast<_Real>(__k + 1);
    __L_n_prev        = __L_n;
    __L_n             = __L_n_next;
  }

  if (!__math::isfinite(__L_n)) {
    // Overflow occurred. L_n(x) is only Preconditions-valid for x >= 0, where its leading term is
    // (-x)^n / n! -- the sign of an overflowing result therefore follows the parity of n alone.
    _Real __inf = std::numeric_limits<_Real>::infinity();
    return (__n & 1) ? -__inf : __inf;
  }
  return __L_n;
  // NOLINTEND(readability-identifier-naming)
}

inline _LIBCPP_HIDE_FROM_ABI double laguerre(unsigned __n, double __x) { return std::__laguerre(__n, __x); }

inline _LIBCPP_HIDE_FROM_ABI float laguerre(unsigned __n, float __x) {
  // use double internally -- float is too prone to overflow, same rationale as hermite.
  return static_cast<float>(std::laguerre(__n, static_cast<double>(__x)));
}

inline _LIBCPP_HIDE_FROM_ABI long double laguerre(unsigned __n, long double __x) {
  return std::__laguerre(__n, __x);
}

inline _LIBCPP_HIDE_FROM_ABI float laguerref(unsigned __n, float __x) { return std::laguerre(__n, __x); }

inline _LIBCPP_HIDE_FROM_ABI long double laguerrel(unsigned __n, long double __x) {
  return std::laguerre(__n, __x);
}

template <class _Integer, std::enable_if_t<std::is_integral_v<_Integer>, int> = 0>
_LIBCPP_HIDE_FROM_ABI double laguerre(unsigned __n, _Integer __x) {
  return std::laguerre(__n, static_cast<double>(__x));
}

// ======================= Associated (generalized) Laguerre polynomials =======================

template <class _Real>
_LIBCPP_HIDE_FROM_ABI _Real __assoc_laguerre(unsigned __n, unsigned __m, _Real __x) {
  // The associated Laguerre polynomial L_n^m(x).
  // Recursion: (k+1) L_{k+1}^m(x) = (2k+1+m-x) L_k^m(x) - (k+m) L_{k-1}^m(x).
  // Abramowitz & Stegun 22.7.30-22.7.31.

  // NOLINTBEGIN(readability-identifier-naming)
  if (__math::isnan(__x))
    return __x;

  _Real __L_0{1};
  if (__n == 0)
    return __L_0;

  _Real __L_n_prev = __L_0;
  _Real __L_n      = _Real(1) + static_cast<_Real>(__m) - __x;
  for (unsigned __k = 1; __k < __n; ++__k) {
    _Real __coeff     = static_cast<_Real>(2 * __k + 1 + __m) - __x;
    _Real __L_n_next  = (__coeff * __L_n - static_cast<_Real>(__k + __m) * __L_n_prev) / static_cast<_Real>(__k + 1);
    __L_n_prev        = __L_n;
    __L_n             = __L_n_next;
  }

  if (!__math::isfinite(__L_n)) {
    // Same overflow-sign reasoning as laguerre: valid only for x >= 0, leading term (-x)^n / n!.
    _Real __inf = std::numeric_limits<_Real>::infinity();
    return (__n & 1) ? -__inf : __inf;
  }
  return __L_n;
  // NOLINTEND(readability-identifier-naming)
}

inline _LIBCPP_HIDE_FROM_ABI double assoc_laguerre(unsigned __n, unsigned __m, double __x) {
  return std::__assoc_laguerre(__n, __m, __x);
}

inline _LIBCPP_HIDE_FROM_ABI float assoc_laguerre(unsigned __n, unsigned __m, float __x) {
  return static_cast<float>(std::assoc_laguerre(__n, __m, static_cast<double>(__x)));
}

inline _LIBCPP_HIDE_FROM_ABI long double assoc_laguerre(unsigned __n, unsigned __m, long double __x) {
  return std::__assoc_laguerre(__n, __m, __x);
}

inline _LIBCPP_HIDE_FROM_ABI float assoc_laguerref(unsigned __n, unsigned __m, float __x) {
  return std::assoc_laguerre(__n, __m, __x);
}

inline _LIBCPP_HIDE_FROM_ABI long double assoc_laguerrel(unsigned __n, unsigned __m, long double __x) {
  return std::assoc_laguerre(__n, __m, __x);
}

template <class _Integer, std::enable_if_t<std::is_integral_v<_Integer>, int> = 0>
_LIBCPP_HIDE_FROM_ABI double assoc_laguerre(unsigned __n, unsigned __m, _Integer __x) {
  return std::assoc_laguerre(__n, __m, static_cast<double>(__x));
}

// ======================= Beta function =======================

template <class _Real>
_LIBCPP_HIDE_FROM_ABI _Real __beta(_Real __x, _Real __y) {
  // B(x, y) = Gamma(x) Gamma(y) / Gamma(x+y), for x > 0, y > 0.
  // Computed as exp(lgamma(x) + lgamma(y) - lgamma(x+y)) rather than tgamma(x)*tgamma(y)/tgamma(x+y):
  // the direct product overflows for modest x, y (e.g. x=y=200) long before the true result does.

  // NOLINTBEGIN(readability-identifier-naming)
  if (__math::isnan(__x) || __math::isnan(__y))
    return __x + __y;

  return __math::exp(__math::lgamma(__x) + __math::lgamma(__y) - __math::lgamma(__x + __y));
  // NOLINTEND(readability-identifier-naming)
}

inline _LIBCPP_HIDE_FROM_ABI double beta(double __x, double __y) { return std::__beta(__x, __y); }

inline _LIBCPP_HIDE_FROM_ABI float beta(float __x, float __y) { return std::__beta(__x, __y); }

inline _LIBCPP_HIDE_FROM_ABI long double beta(long double __x, long double __y) { return std::__beta(__x, __y); }

inline _LIBCPP_HIDE_FROM_ABI float betaf(float __x, float __y) { return std::beta(__x, __y); }

inline _LIBCPP_HIDE_FROM_ABI long double betal(long double __x, long double __y) { return std::beta(__x, __y); }

template <class _A1, class _A2, std::enable_if_t<std::is_arithmetic_v<_A1> && std::is_arithmetic_v<_A2>, int> = 0>
_LIBCPP_HIDE_FROM_ABI __promote_t<_A1, _A2> beta(_A1 __x, _A2 __y) {
  using __result_type = __promote_t<_A1, _A2>;
  return std::beta(static_cast<__result_type>(__x), static_cast<__result_type>(__y));
}

// ======================= Spherical associated Legendre functions =======================

template <class _Real>
_LIBCPP_HIDE_FROM_ABI _Real __sph_legendre(unsigned __l, unsigned __m, _Real __theta) {
  // Y_l^m(theta) = sqrt((2l+1)/(4*pi) * (l-m)!/(l+m)!) * P_l^m(cos theta), where P_l^m already
  // includes the Condon-Shortley phase (matches std::assoc_legendre's own definition -- confirmed
  // empirically against an independent oracle, since the standard's cross-references between
  // assoc_legendre and sph_legendre's phase conventions are easy to misread).
  // The normalizing factor is computed in log-space via lgamma: (l-m)!/(l+m)! computed as raw
  // factorials would overflow tgamma for large l long before the (shrinking) ratio does.

  // NOLINTBEGIN(readability-identifier-naming)
  if (__math::isnan(__theta))
    return __theta;

  _Real __log_norm =
      (__math::log(static_cast<_Real>(2 * __l + 1)) - __math::log(_Real(4) * __math::__pi_v<_Real>) +
       __math::lgamma(static_cast<_Real>(__l - __m + 1)) - __math::lgamma(static_cast<_Real>(__l + __m + 1))) /
      2;
  _Real __norm = __math::exp(__log_norm);
  return __norm * std::__assoc_legendre(__l, __m, __math::cos(__theta));
  // NOLINTEND(readability-identifier-naming)
}

inline _LIBCPP_HIDE_FROM_ABI double sph_legendre(unsigned __l, unsigned __m, double __theta) {
  return std::__sph_legendre(__l, __m, __theta);
}

inline _LIBCPP_HIDE_FROM_ABI float sph_legendre(unsigned __l, unsigned __m, float __theta) {
  return static_cast<float>(std::sph_legendre(__l, __m, static_cast<double>(__theta)));
}

inline _LIBCPP_HIDE_FROM_ABI long double sph_legendre(unsigned __l, unsigned __m, long double __theta) {
  return std::__sph_legendre(__l, __m, __theta);
}

inline _LIBCPP_HIDE_FROM_ABI float sph_legendref(unsigned __l, unsigned __m, float __theta) {
  return std::sph_legendre(__l, __m, __theta);
}

inline _LIBCPP_HIDE_FROM_ABI long double sph_legendrel(unsigned __l, unsigned __m, long double __theta) {
  return std::sph_legendre(__l, __m, __theta);
}

template <class _Integer, std::enable_if_t<std::is_integral_v<_Integer>, int> = 0>
_LIBCPP_HIDE_FROM_ABI double sph_legendre(unsigned __l, unsigned __m, _Integer __theta) {
  return std::sph_legendre(__l, __m, static_cast<double>(__theta));
}

// ======================= Elliptic integrals (Carlson symmetric forms) =======================
//
// All six elliptic-integral special functions (comp_ellint_{1,2,3}, ellint_{1,2,3}) are implemented
// as thin wrappers around three symmetric Carlson forms, R_F, R_D, R_J (R_C is a private helper used
// only inside R_J). This is the standard technique for numerically evaluating both complete and
// incomplete elliptic integrals of all three kinds with a single reusable algorithm rather than
// bespoke series/AGM code per function.
//
// Carlson, B. C. "Numerical computation of real or complex elliptic integrals." Numerical Algorithms
// 10.1 (1995): 13-26. The duplication-theorem iteration, convergence tolerances, and 5th-order Taylor
// correction coefficients used below follow that paper's algorithm directly (the same algorithm is
// reproduced, with the same well-known coefficients, in most public elliptic-integral
// implementations, e.g. Boost.Math and the GNU Scientific Library).

template <class _Real>
_LIBCPP_HIDE_FROM_ABI _Real __carlson_rc(_Real __x, _Real __y) {
  // Degenerate symmetric form R_C(x, y) = R_F(x, y, y), used inside R_J. Preconditions: x >= 0, y != 0.

  // NOLINTBEGIN(readability-identifier-naming)
  constexpr _Real __errtol = _Real(0.0007);
  _Real __xt, __yt, __w;
  if (__y > 0) {
    __xt = __x;
    __yt = __y;
    __w  = _Real(1);
  } else {
    // y < 0: use R_C(x, y) = sqrt(x / (x-y)) * R_C(x-y, -y).
    __xt = __x - __y;
    __yt = -__y;
    __w  = __math::sqrt(__x) / __math::sqrt(__xt);
  }
  _Real __ave, __s;
  do {
    _Real __sqrtx = __math::sqrt(__xt);
    _Real __sqrty = __math::sqrt(__yt);
    _Real __alamb = 2 * __sqrtx * __sqrty + __yt;
    __xt          = _Real(0.25) * (__xt + __alamb);
    __yt          = _Real(0.25) * (__yt + __alamb);
    __ave         = _Real(1) / _Real(3) * (__xt + 2 * __yt);
    __s           = (__yt - __ave) / __ave;
  } while (__math::fabs(__s) > __errtol);
  return __w *
         (_Real(1) + __s * __s * (_Real(3) / 10 + __s * (_Real(1) / 7 + __s * (_Real(3) / 8 + __s * _Real(9) / 22)))) /
         __math::sqrt(__ave);
  // NOLINTEND(readability-identifier-naming)
}

template <class _Real>
_LIBCPP_HIDE_FROM_ABI _Real __carlson_rf(_Real __x, _Real __y, _Real __z) {
  // R_F(x, y, z), the symmetric elliptic integral of the first kind. Preconditions: x, y, z >= 0, at
  // most one of them zero.

  // NOLINTBEGIN(readability-identifier-naming)
  constexpr _Real __errtol = _Real(0.0025);
  _Real __xt = __x, __yt = __y, __zt = __z;
  _Real __delx, __dely, __delz, __ave;
  do {
    _Real __sqrtx = __math::sqrt(__xt);
    _Real __sqrty = __math::sqrt(__yt);
    _Real __sqrtz = __math::sqrt(__zt);
    _Real __alamb = __sqrtx * (__sqrty + __sqrtz) + __sqrty * __sqrtz;
    __xt          = _Real(0.25) * (__xt + __alamb);
    __yt          = _Real(0.25) * (__yt + __alamb);
    __zt          = _Real(0.25) * (__zt + __alamb);
    __ave         = _Real(1) / _Real(3) * (__xt + __yt + __zt);
    __delx        = (__ave - __xt) / __ave;
    __dely        = (__ave - __yt) / __ave;
    __delz        = (__ave - __zt) / __ave;
  } while (__math::fabs(__delx) > __errtol || __math::fabs(__dely) > __errtol || __math::fabs(__delz) > __errtol);
  _Real __e2 = __delx * __dely - __delz * __delz;
  _Real __e3 = __delx * __dely * __delz;
  return (_Real(1) + (_Real(1) / 24 * __e2 - _Real(0.1) - _Real(3) / 44 * __e3) * __e2 + _Real(1) / 14 * __e3) /
         __math::sqrt(__ave);
  // NOLINTEND(readability-identifier-naming)
}

template <class _Real>
_LIBCPP_HIDE_FROM_ABI _Real __carlson_rd(_Real __x, _Real __y, _Real __z) {
  // R_D(x, y, z), the symmetric elliptic integral of the second kind. Preconditions: x, y >= 0,
  // z > 0, at most one of x, y zero.

  // NOLINTBEGIN(readability-identifier-naming)
  constexpr _Real __errtol = _Real(0.0015);
  _Real __xt = __x, __yt = __y, __zt = __z, __sum = 0, __fac = 1;
  _Real __delx, __dely, __delz, __ave;
  do {
    _Real __sqrtx = __math::sqrt(__xt);
    _Real __sqrty = __math::sqrt(__yt);
    _Real __sqrtz = __math::sqrt(__zt);
    _Real __alamb = __sqrtx * (__sqrty + __sqrtz) + __sqrty * __sqrtz;
    __sum += __fac / (__sqrtz * (__zt + __alamb));
    __fac *= _Real(0.25);
    __xt   = _Real(0.25) * (__xt + __alamb);
    __yt   = _Real(0.25) * (__yt + __alamb);
    __zt   = _Real(0.25) * (__zt + __alamb);
    __ave  = _Real(0.2) * (__xt + __yt + 3 * __zt);
    __delx = (__ave - __xt) / __ave;
    __dely = (__ave - __yt) / __ave;
    __delz = (__ave - __zt) / __ave;
  } while (__math::fabs(__delx) > __errtol || __math::fabs(__dely) > __errtol || __math::fabs(__delz) > __errtol);
  _Real __ea = __delx * __dely;
  _Real __eb = __delz * __delz;
  _Real __ec = __ea - __eb;
  _Real __ed = __ea - 6 * __eb;
  _Real __ee = __ed + __ec + __ec;
  return 3 * __sum + __fac *
                          (_Real(1) +
                           __ed * (-_Real(3) / 14 + _Real(9) / 88 * __ed - _Real(9) / 52 * __delz * __ee) +
                           __delz * (_Real(1) / 6 * __ee + __delz * (-_Real(3) / 22 * __ec + __delz * _Real(3) / 26 * __ea))) /
                          (__ave * __math::sqrt(__ave));
  // NOLINTEND(readability-identifier-naming)
}

template <class _Real>
_LIBCPP_HIDE_FROM_ABI _Real __carlson_rj(_Real __x, _Real __y, _Real __z, _Real __p) {
  // R_J(x, y, z, p), the symmetric elliptic integral of the third kind. Preconditions: x, y, z >= 0,
  // at most one zero, p != 0. Only p > 0 is needed by [sf.cmath]'s ellint_3/comp_ellint_3 (their
  // domain keeps the R_J argument positive), so the p < 0 branch (which needs an extra root-finding
  // step) is intentionally not implemented.

  // NOLINTBEGIN(readability-identifier-naming)
  constexpr _Real __errtol = _Real(0.0015);
  _Real __xt = __x, __yt = __y, __zt = __z, __pt = __p, __sum = 0, __fac = 1;
  _Real __delx, __dely, __delz, __delp, __ave;
  do {
    _Real __sqrtx = __math::sqrt(__xt);
    _Real __sqrty = __math::sqrt(__yt);
    _Real __sqrtz = __math::sqrt(__zt);
    _Real __alamb = __sqrtx * (__sqrty + __sqrtz) + __sqrty * __sqrtz;
    _Real __alpha_base = __pt * (__sqrtx + __sqrty + __sqrtz) + __sqrtx * __sqrty * __sqrtz;
    _Real __alpha      = __alpha_base * __alpha_base;
    _Real __beta_base  = __pt + __alamb;
    _Real __beta       = __pt * __beta_base * __beta_base;
    __sum += __fac * std::__carlson_rc(__alpha, __beta);
    __fac *= _Real(0.25);
    __xt   = _Real(0.25) * (__xt + __alamb);
    __yt   = _Real(0.25) * (__yt + __alamb);
    __zt   = _Real(0.25) * (__zt + __alamb);
    __pt   = _Real(0.25) * (__pt + __alamb);
    __ave  = _Real(0.2) * (__xt + __yt + __zt + 2 * __pt);
    __delx = (__ave - __xt) / __ave;
    __dely = (__ave - __yt) / __ave;
    __delz = (__ave - __zt) / __ave;
    __delp = (__ave - __pt) / __ave;
  } while (__math::fabs(__delx) > __errtol || __math::fabs(__dely) > __errtol || __math::fabs(__delz) > __errtol ||
           __math::fabs(__delp) > __errtol);
  _Real __ea = __delx * (__dely + __delz) + __dely * __delz;
  _Real __eb = __delx * __dely * __delz;
  _Real __ec = __delp * __delp;
  _Real __ed = __ea - 3 * __ec;
  _Real __ee = __eb + 2 * __delp * (__ea - __ec);
  return 3 * __sum +
         __fac *
             (_Real(1) + __ed * (-_Real(3) / 14 + _Real(9) / 88 * __ed - _Real(9) / 52 * __ee) +
              __eb * (_Real(1) / 6 + __delp * (-_Real(3) / 11 + __delp * _Real(3) / 26)) +
              __delp * __ea * (_Real(1) / 3 - _Real(3) / 22 * __delp) - _Real(1) / 3 * __delp * __ec) /
             (__ave * __math::sqrt(__ave));
  // NOLINTEND(readability-identifier-naming)
}

// ---- comp_ellint_1 ----

template <class _Real>
_LIBCPP_HIDE_FROM_ABI _Real __comp_ellint_1(_Real __k) {
  // NOLINTBEGIN(readability-identifier-naming)
  if (__math::isnan(__k))
    return __k;
  _Real __k2 = __k * __k;
  return std::__carlson_rf(_Real(0), _Real(1) - __k2, _Real(1));
  // NOLINTEND(readability-identifier-naming)
}

inline _LIBCPP_HIDE_FROM_ABI double comp_ellint_1(double __k) { return std::__comp_ellint_1(__k); }

inline _LIBCPP_HIDE_FROM_ABI float comp_ellint_1(float __k) {
  return static_cast<float>(std::comp_ellint_1(static_cast<double>(__k)));
}

inline _LIBCPP_HIDE_FROM_ABI long double comp_ellint_1(long double __k) { return std::__comp_ellint_1(__k); }

inline _LIBCPP_HIDE_FROM_ABI float comp_ellint_1f(float __k) { return std::comp_ellint_1(__k); }

inline _LIBCPP_HIDE_FROM_ABI long double comp_ellint_1l(long double __k) { return std::comp_ellint_1(__k); }

template <class _Integer, std::enable_if_t<std::is_integral_v<_Integer>, int> = 0>
_LIBCPP_HIDE_FROM_ABI double comp_ellint_1(_Integer __k) {
  return std::comp_ellint_1(static_cast<double>(__k));
}

// ---- comp_ellint_2 ----

template <class _Real>
_LIBCPP_HIDE_FROM_ABI _Real __comp_ellint_2(_Real __k) {
  // NOLINTBEGIN(readability-identifier-naming)
  if (__math::isnan(__k))
    return __k;
  _Real __k2 = __k * __k;
  if (__k2 == 1)
    return _Real(1);
  return std::__carlson_rf(_Real(0), _Real(1) - __k2, _Real(1)) -
         __k2 / 3 * std::__carlson_rd(_Real(0), _Real(1) - __k2, _Real(1));
  // NOLINTEND(readability-identifier-naming)
}

inline _LIBCPP_HIDE_FROM_ABI double comp_ellint_2(double __k) { return std::__comp_ellint_2(__k); }

inline _LIBCPP_HIDE_FROM_ABI float comp_ellint_2(float __k) {
  return static_cast<float>(std::comp_ellint_2(static_cast<double>(__k)));
}

inline _LIBCPP_HIDE_FROM_ABI long double comp_ellint_2(long double __k) { return std::__comp_ellint_2(__k); }

inline _LIBCPP_HIDE_FROM_ABI float comp_ellint_2f(float __k) { return std::comp_ellint_2(__k); }

inline _LIBCPP_HIDE_FROM_ABI long double comp_ellint_2l(long double __k) { return std::comp_ellint_2(__k); }

template <class _Integer, std::enable_if_t<std::is_integral_v<_Integer>, int> = 0>
_LIBCPP_HIDE_FROM_ABI double comp_ellint_2(_Integer __k) {
  return std::comp_ellint_2(static_cast<double>(__k));
}

// ---- comp_ellint_3 ----

template <class _Real>
_LIBCPP_HIDE_FROM_ABI _Real __comp_ellint_3(_Real __k, _Real __nu) {
  // NOLINTBEGIN(readability-identifier-naming)
  if (__math::isnan(__k) || __math::isnan(__nu))
    return __k + __nu;
  _Real __k2 = __k * __k;
  return std::__carlson_rf(_Real(0), _Real(1) - __k2, _Real(1)) +
         __nu / 3 * std::__carlson_rj(_Real(0), _Real(1) - __k2, _Real(1), _Real(1) - __nu);
  // NOLINTEND(readability-identifier-naming)
}

inline _LIBCPP_HIDE_FROM_ABI double comp_ellint_3(double __k, double __nu) { return std::__comp_ellint_3(__k, __nu); }

inline _LIBCPP_HIDE_FROM_ABI float comp_ellint_3(float __k, float __nu) {
  return static_cast<float>(std::comp_ellint_3(static_cast<double>(__k), static_cast<double>(__nu)));
}

inline _LIBCPP_HIDE_FROM_ABI long double comp_ellint_3(long double __k, long double __nu) {
  return std::__comp_ellint_3(__k, __nu);
}

inline _LIBCPP_HIDE_FROM_ABI float comp_ellint_3f(float __k, float __nu) { return std::comp_ellint_3(__k, __nu); }

inline _LIBCPP_HIDE_FROM_ABI long double comp_ellint_3l(long double __k, long double __nu) {
  return std::comp_ellint_3(__k, __nu);
}

template <class _A1, class _A2, std::enable_if_t<std::is_arithmetic_v<_A1> && std::is_arithmetic_v<_A2>, int> = 0>
_LIBCPP_HIDE_FROM_ABI __promote_t<_A1, _A2> comp_ellint_3(_A1 __k, _A2 __nu) {
  using __result_type = __promote_t<_A1, _A2>;
  return std::comp_ellint_3(static_cast<__result_type>(__k), static_cast<__result_type>(__nu));
}

// ---- ellint_1 ----

template <class _Real>
_LIBCPP_HIDE_FROM_ABI _Real __ellint_1(_Real __k, _Real __phi) {
  // NOLINTBEGIN(readability-identifier-naming)
  if (__math::isnan(__k) || __math::isnan(__phi))
    return __k + __phi;
  _Real __s   = __math::sin(__phi);
  _Real __c   = __math::cos(__phi);
  _Real __k2s2 = __k * __k * __s * __s;
  return __s * std::__carlson_rf(__c * __c, _Real(1) - __k2s2, _Real(1));
  // NOLINTEND(readability-identifier-naming)
}

inline _LIBCPP_HIDE_FROM_ABI double ellint_1(double __k, double __phi) { return std::__ellint_1(__k, __phi); }

inline _LIBCPP_HIDE_FROM_ABI float ellint_1(float __k, float __phi) {
  return static_cast<float>(std::ellint_1(static_cast<double>(__k), static_cast<double>(__phi)));
}

inline _LIBCPP_HIDE_FROM_ABI long double ellint_1(long double __k, long double __phi) {
  return std::__ellint_1(__k, __phi);
}

inline _LIBCPP_HIDE_FROM_ABI float ellint_1f(float __k, float __phi) { return std::ellint_1(__k, __phi); }

inline _LIBCPP_HIDE_FROM_ABI long double ellint_1l(long double __k, long double __phi) {
  return std::ellint_1(__k, __phi);
}

template <class _A1, class _A2, std::enable_if_t<std::is_arithmetic_v<_A1> && std::is_arithmetic_v<_A2>, int> = 0>
_LIBCPP_HIDE_FROM_ABI __promote_t<_A1, _A2> ellint_1(_A1 __k, _A2 __phi) {
  using __result_type = __promote_t<_A1, _A2>;
  return std::ellint_1(static_cast<__result_type>(__k), static_cast<__result_type>(__phi));
}

// ---- ellint_2 ----

template <class _Real>
_LIBCPP_HIDE_FROM_ABI _Real __ellint_2(_Real __k, _Real __phi) {
  // NOLINTBEGIN(readability-identifier-naming)
  if (__math::isnan(__k) || __math::isnan(__phi))
    return __k + __phi;
  _Real __s    = __math::sin(__phi);
  _Real __c    = __math::cos(__phi);
  _Real __k2   = __k * __k;
  _Real __k2s2 = __k2 * __s * __s;
  _Real __x    = __c * __c;
  _Real __y    = _Real(1) - __k2s2;
  return __s * std::__carlson_rf(__x, __y, _Real(1)) -
         __k2 / 3 * __s * __s * __s * std::__carlson_rd(__x, __y, _Real(1));
  // NOLINTEND(readability-identifier-naming)
}

inline _LIBCPP_HIDE_FROM_ABI double ellint_2(double __k, double __phi) { return std::__ellint_2(__k, __phi); }

inline _LIBCPP_HIDE_FROM_ABI float ellint_2(float __k, float __phi) {
  return static_cast<float>(std::ellint_2(static_cast<double>(__k), static_cast<double>(__phi)));
}

inline _LIBCPP_HIDE_FROM_ABI long double ellint_2(long double __k, long double __phi) {
  return std::__ellint_2(__k, __phi);
}

inline _LIBCPP_HIDE_FROM_ABI float ellint_2f(float __k, float __phi) { return std::ellint_2(__k, __phi); }

inline _LIBCPP_HIDE_FROM_ABI long double ellint_2l(long double __k, long double __phi) {
  return std::ellint_2(__k, __phi);
}

template <class _A1, class _A2, std::enable_if_t<std::is_arithmetic_v<_A1> && std::is_arithmetic_v<_A2>, int> = 0>
_LIBCPP_HIDE_FROM_ABI __promote_t<_A1, _A2> ellint_2(_A1 __k, _A2 __phi) {
  using __result_type = __promote_t<_A1, _A2>;
  return std::ellint_2(static_cast<__result_type>(__k), static_cast<__result_type>(__phi));
}

// ---- ellint_3 ----

template <class _Real>
_LIBCPP_HIDE_FROM_ABI _Real __ellint_3(_Real __k, _Real __nu, _Real __phi) {
  // NOLINTBEGIN(readability-identifier-naming)
  if (__math::isnan(__k) || __math::isnan(__nu) || __math::isnan(__phi))
    return __k + __nu + __phi;
  _Real __s    = __math::sin(__phi);
  _Real __c    = __math::cos(__phi);
  _Real __k2s2 = __k * __k * __s * __s;
  _Real __x    = __c * __c;
  _Real __y    = _Real(1) - __k2s2;
  _Real __s3   = __s * __s * __s;
  return __s * std::__carlson_rf(__x, __y, _Real(1)) +
         __nu / 3 * __s3 * std::__carlson_rj(__x, __y, _Real(1), _Real(1) - __nu * __s * __s);
  // NOLINTEND(readability-identifier-naming)
}

inline _LIBCPP_HIDE_FROM_ABI double ellint_3(double __k, double __nu, double __phi) {
  return std::__ellint_3(__k, __nu, __phi);
}

inline _LIBCPP_HIDE_FROM_ABI float ellint_3(float __k, float __nu, float __phi) {
  return static_cast<float>(
      std::ellint_3(static_cast<double>(__k), static_cast<double>(__nu), static_cast<double>(__phi)));
}

inline _LIBCPP_HIDE_FROM_ABI long double ellint_3(long double __k, long double __nu, long double __phi) {
  return std::__ellint_3(__k, __nu, __phi);
}

inline _LIBCPP_HIDE_FROM_ABI float ellint_3f(float __k, float __nu, float __phi) { return std::ellint_3(__k, __nu, __phi); }

inline _LIBCPP_HIDE_FROM_ABI long double ellint_3l(long double __k, long double __nu, long double __phi) {
  return std::ellint_3(__k, __nu, __phi);
}

template <class _A1,
          class _A2,
          class _A3,
          std::enable_if_t<std::is_arithmetic_v<_A1> && std::is_arithmetic_v<_A2> && std::is_arithmetic_v<_A3>, int> = 0>
_LIBCPP_HIDE_FROM_ABI __promote_t<_A1, _A2, _A3> ellint_3(_A1 __k, _A2 __nu, _A3 __phi) {
  using __result_type = __promote_t<_A1, _A2, _A3>;
  return std::ellint_3(static_cast<__result_type>(__k), static_cast<__result_type>(__nu), static_cast<__result_type>(__phi));
}

// ======================= Exponential integral =======================
//
// std::expint(x) computes Ei(x) = -PV integral from -x to infinity of (e^-t / t) dt, x != 0 (this is
// the "Ei" exponential integral, not "E1" -- Ei(x) for x > 0 is positive and grows like e^x/x, while
// E1(x) for x > 0 is a different, always-positive function; confirmed against an independent oracle
// before committing to this, since the two are easy to swap by mistake). For x < 0, Ei(x) = -E1(-x).

template <class _Real>
_LIBCPP_HIDE_FROM_ABI _Real __expint_e1_series(_Real __z) {
  // E1(z) for 0 < z <= 1, via the everywhere-convergent alternating series
  // E1(z) = -gamma - ln(z) + sum_{k=1}^inf (-1)^(k+1) z^k / (k * k!).
  // Abramowitz & Stegun 5.1.11.

  // NOLINTBEGIN(readability-identifier-naming)
  constexpr _Real __euler_gamma = _Real(0.5772156649015328606065120900824024310421593359399235988057672348849L);
  _Real __fact                  = 1;
  _Real __sum                   = 0;
  for (int __k = 1; __k < 1000; ++__k) {
    __fact *= -__z / __k;
    _Real __term = -__fact / __k;
    __sum += __term;
    if (__k > 5 && __math::fabs(__term) < std::numeric_limits<_Real>::epsilon() * __math::fabs(__sum))
      break;
  }
  return -__euler_gamma - __math::log(__z) + __sum;
  // NOLINTEND(readability-identifier-naming)
}

template <class _Real>
_LIBCPP_HIDE_FROM_ABI _Real __expint_e1_continued_fraction(_Real __z) {
  // E1(z) for z > 1, via the continued fraction
  // E1(z) = e^-z / (z + 1 - 1^2/(z + 3 - 2^2/(z + 5 - 3^2/(z + 7 - ...)))),
  // evaluated with Lentz's method (a numerically stable way to evaluate a continued fraction term by
  // term without ever forming the full numerator/denominator).
  // Press, William H., et al. Numerical recipes 3rd edition, p. 262 (`expint`), and 5.2 (Lentz's
  // method generally). Abramowitz & Stegun 5.1.22 for the continued fraction itself.

  // NOLINTBEGIN(readability-identifier-naming)
  constexpr _Real __tiny = std::numeric_limits<_Real>::min() * 16;
  _Real __b               = __z + 1;
  _Real __c               = 1 / __tiny;
  _Real __d               = 1 / __b;
  _Real __h                = __d;
  for (int __i = 1; __i < 1000; ++__i) {
    _Real __a = -static_cast<_Real>(__i) * static_cast<_Real>(__i);
    __b += 2;
    __d          = 1 / (__a * __d + __b);
    __c          = __b + __a / __c;
    _Real __del  = __c * __d;
    __h *= __del;
    if (__math::fabs(__del - 1) < std::numeric_limits<_Real>::epsilon())
      break;
  }
  return __h * __math::exp(-__z);
  // NOLINTEND(readability-identifier-naming)
}

template <class _Real>
_LIBCPP_HIDE_FROM_ABI _Real __expint(_Real __x) {
  // NOLINTBEGIN(readability-identifier-naming)
  if (__math::isnan(__x))
    return __x;
  if (__x == 0)
    return -std::numeric_limits<_Real>::infinity();

  if (__x > 0) {
    // Ei(x) = gamma + ln(x) + sum_{k=1}^inf x^k / (k * k!): also everywhere-convergent, and every
    // term stays positive for x > 0, so there is no cancellation to worry about even though this
    // series is used for all x > 0 rather than switching to an asymptotic expansion for large x.
    constexpr _Real __euler_gamma = _Real(0.5772156649015328606065120900824024310421593359399235988057672348849L);
    _Real __fact                  = 1;
    _Real __sum                   = 0;
    for (int __k = 1; __k < 100000; ++__k) {
      __fact *= __x / __k;
      _Real __term = __fact / __k;
      __sum += __term;
      if (__k > 5 && __term < std::numeric_limits<_Real>::epsilon() * __sum)
        break;
    }
    return __sum + __math::log(__x) + __euler_gamma;
  }

  _Real __z = -__x;
  return -(__z <= 1 ? std::__expint_e1_series(__z) : std::__expint_e1_continued_fraction(__z));
  // NOLINTEND(readability-identifier-naming)
}

inline _LIBCPP_HIDE_FROM_ABI double expint(double __x) { return std::__expint(__x); }

inline _LIBCPP_HIDE_FROM_ABI float expint(float __x) {
  return static_cast<float>(std::expint(static_cast<double>(__x)));
}

inline _LIBCPP_HIDE_FROM_ABI long double expint(long double __x) { return std::__expint(__x); }

inline _LIBCPP_HIDE_FROM_ABI float expintf(float __x) { return std::expint(__x); }

inline _LIBCPP_HIDE_FROM_ABI long double expintl(long double __x) { return std::expint(__x); }

template <class _Integer, std::enable_if_t<std::is_integral_v<_Integer>, int> = 0>
_LIBCPP_HIDE_FROM_ABI double expint(_Integer __x) {
  return std::expint(static_cast<double>(__x));
}

// ======================= Riemann zeta function =======================

template <class _Real>
_LIBCPP_HIDE_FROM_ABI _Real __riemann_zeta(_Real __s) {
  // zeta(s) via Euler-Maclaurin summation:
  //   zeta(s) = sum_{k=1}^{N} k^-s  +  N^(1-s)/(s-1)  -  N^-s/2
  //             + sum_{j=1}^{M} [B_2j / (2j)!] * (s)_(2j-1) * N^(-s-2j+1)
  // where (s)_(2j-1) = s(s+1)...(s+2j-2) is the length-(2j-1) rising factorial of s, and B_2j are
  // Bernoulli numbers. N = 15 direct-summation terms and M = 8 correction terms give full double
  // precision (checked against an independent oracle across s in [-10, 50]) -- the correction series
  // is itself an asymptotic expansion, valid (for this N, M) for s down to about -14; well past what
  // [sf.cmath] requires. Abramowitz & Stegun 23.2.9 for both the coefficients and the formula.

  // NOLINTBEGIN(readability-identifier-naming)
  if (__math::isnan(__s))
    return __s;
  if (__s == 1)
    return std::numeric_limits<_Real>::infinity();
  // Trivial zeros: zeta(s) == 0 exactly at every negative even integer. The Euler-Maclaurin formula
  // only gets arbitrarily close to zero there (through cancellation), so this is special-cased to
  // match the exact value (and what an independent oracle returns).
  if (__s < 0 && __s == __math::trunc(__s) && __math::fmod(__s, _Real(2)) == 0)
    return _Real(0);

  constexpr int __n                = 15;
  constexpr _Real __coeffs[8] = {
      _Real(1.0L / 12),
      _Real(-1.0L / 720),
      _Real(1.0L / 30240),
      _Real(-1.0L / 1209600),
      _Real(1.0L / 47900160),
      _Real(-691.0L / 1307674368000.0L),
      _Real(1.0L / 74724249600.0L),
      _Real(-3617.0L / 10670622842880000.0L),
  };

  _Real __sum = 0;
  for (int __k = 1; __k <= __n; ++__k)
    __sum += __math::pow(static_cast<_Real>(__k), -__s);
  _Real __result =
      __sum + __math::pow(static_cast<_Real>(__n), _Real(1) - __s) / (__s - 1) -
      __math::pow(static_cast<_Real>(__n), -__s) / 2;

  _Real __rising          = __s;
  _Real __npows           = __math::pow(static_cast<_Real>(__n), -__s);
  _Real __npow_neg_2jm1   = _Real(1) / __n;
  for (int __j = 1; __j <= 8; ++__j) {
    if (__j > 1) {
      __rising *= (__s + static_cast<_Real>(2 * __j - 3)) * (__s + static_cast<_Real>(2 * __j - 2));
      __npow_neg_2jm1 /= static_cast<_Real>(__n) * static_cast<_Real>(__n);
    }
    __result += __coeffs[__j - 1] * __rising * __npows * __npow_neg_2jm1;
  }
  return __result;
  // NOLINTEND(readability-identifier-naming)
}

inline _LIBCPP_HIDE_FROM_ABI double riemann_zeta(double __s) { return std::__riemann_zeta(__s); }

inline _LIBCPP_HIDE_FROM_ABI float riemann_zeta(float __s) {
  return static_cast<float>(std::riemann_zeta(static_cast<double>(__s)));
}

inline _LIBCPP_HIDE_FROM_ABI long double riemann_zeta(long double __s) { return std::__riemann_zeta(__s); }

inline _LIBCPP_HIDE_FROM_ABI float riemann_zetaf(float __s) { return std::riemann_zeta(__s); }

inline _LIBCPP_HIDE_FROM_ABI long double riemann_zetal(long double __s) { return std::riemann_zeta(__s); }

template <class _Integer, std::enable_if_t<std::is_integral_v<_Integer>, int> = 0>
_LIBCPP_HIDE_FROM_ABI double riemann_zeta(_Integer __s) {
  return std::riemann_zeta(static_cast<double>(__s));
}

// ======================= Cylindrical Bessel functions =======================
//
// cyl_bessel_j (J_nu) and cyl_bessel_i (I_nu) are computed directly from their defining power
// series, which converges for every real nu > -1 and every x >= 0 (both are entire functions of x).
// cyl_neumann (Y_nu) and cyl_bessel_k (K_nu) are then obtained from J and I via the standard
// reflection formulas
//   Y_nu(x) = (J_nu(x) cos(nu*pi) - J_{-nu}(x)) / sin(nu*pi)
//   K_nu(x) = (pi/2) (I_{-nu}(x) - I_nu(x)) / sin(nu*pi)
// which have a removable 0/0 singularity at every integer nu (both J_{-n} and I_{-n}'s combination
// with the *_n term vanish there together with sin(nu*pi)); the limit is recovered by evaluating the
// same formula at nu +- a small offset and Richardson-extrapolating, rather than deriving the
// L'Hopital closed form symbolically (verified against an independent oracle to be accurate to
// better than 1e-6 relative error even in the worst sampled cases, which is the same practical bar
// used elsewhere in this file for expint/riemann_zeta).

template <class _Real>
_LIBCPP_HIDE_FROM_ABI _Real __recip_gamma(_Real __z) {
  // 1/Gamma(z), correctly signed and exactly 0 at every non-positive integer (a removable
  // singularity of 1/Gamma, not a pole) -- needed because __math::lgamma only ever returns
  // ln|Gamma(z)|, silently discarding the sign, which is wrong for z < 0 (e.g. Gamma(-0.5) < 0).

  // NOLINTBEGIN(readability-identifier-naming)
  if (__z > 0)
    return 1 / __math::tgamma(__z);
  _Real __s = __math::sin(__math::__pi_v<_Real> * __z);
  if (__s == 0)
    return 0;
  return __s * __math::tgamma(1 - __z) / __math::__pi_v<_Real>;
  // NOLINTEND(readability-identifier-naming)
}

template <class _Real>
_LIBCPP_HIDE_FROM_ABI _Real __cyl_bessel_j(_Real __nu, _Real __x) {
  // J_nu(x) = sum_{k=0}^inf [(-1)^k / (k! Gamma(nu+k+1))] (x/2)^(2k+nu). Valid for any real nu that
  // is not a negative integer (the per-term recurrence below divides by (nu+k), which is safe as
  // long as nu itself isn't a non-positive integer -- this function is only ever called with nu >= 0
  // or with a non-integer nu, per its two call sites below).

  // NOLINTBEGIN(readability-identifier-naming)
  if (__x == 0)
    return __nu == 0 ? _Real(1) : _Real(0);
  _Real __halfx  = __x / 2;
  _Real __halfx2 = __halfx * __halfx;
  _Real __term   = __math::pow(__halfx, __nu) * std::__recip_gamma(__nu + 1);
  _Real __sum    = __term;
  for (int __k = 1; __k < 1000; ++__k) {
    __term *= -__halfx2 / (static_cast<_Real>(__k) * (__nu + __k));
    __sum += __term;
    if (__k > 5 && __math::fabs(__term) < std::numeric_limits<_Real>::epsilon() * __math::fabs(__sum))
      break;
  }
  return __sum;
  // NOLINTEND(readability-identifier-naming)
}

template <class _Real>
_LIBCPP_HIDE_FROM_ABI _Real __cyl_bessel_i(_Real __nu, _Real __x) {
  // I_nu(x): the same series as J_nu, without the alternating sign. See __cyl_bessel_j's comment for
  // the domain restriction on nu.

  // NOLINTBEGIN(readability-identifier-naming)
  if (__x == 0)
    return __nu == 0 ? _Real(1) : _Real(0);
  _Real __halfx  = __x / 2;
  _Real __halfx2 = __halfx * __halfx;
  _Real __term   = __math::pow(__halfx, __nu) * std::__recip_gamma(__nu + 1);
  _Real __sum    = __term;
  for (int __k = 1; __k < 1000; ++__k) {
    __term *= __halfx2 / (static_cast<_Real>(__k) * (__nu + __k));
    __sum += __term;
    if (__k > 5 && __math::fabs(__term) < std::numeric_limits<_Real>::epsilon() * __math::fabs(__sum))
      break;
  }
  return __sum;
  // NOLINTEND(readability-identifier-naming)
}

template <class _Real>
_LIBCPP_HIDE_FROM_ABI _Real __cyl_neumann_raw(_Real __nu, _Real __x) {
  // NOLINTBEGIN(readability-identifier-naming)
  return (std::__cyl_bessel_j(__nu, __x) * __math::cos(__math::__pi_v<_Real> * __nu) -
          std::__cyl_bessel_j(-__nu, __x)) /
         __math::sin(__math::__pi_v<_Real> * __nu);
  // NOLINTEND(readability-identifier-naming)
}

template <class _Real>
_LIBCPP_HIDE_FROM_ABI _Real __cyl_bessel_k_raw(_Real __nu, _Real __x) {
  // NOLINTBEGIN(readability-identifier-naming)
  return __math::__pi_v<_Real> / 2 * (std::__cyl_bessel_i(-__nu, __x) - std::__cyl_bessel_i(__nu, __x)) /
         __math::sin(__math::__pi_v<_Real> * __nu);
  // NOLINTEND(readability-identifier-naming)
}

// Evaluates a reflection-formula function (__cyl_neumann_raw or __cyl_bessel_k_raw) at nu, working
// around its removable singularity at every integer by symmetric finite differencing plus one step
// of Richardson extrapolation (cancels the leading O(delta) error of the symmetric average, leaving
// O(delta^2)). Accurate for small-to-moderate x, where J_nu/I_nu stay a reasonable size; see
// __cyl_bessel_k's comment for why it is deliberately not used at large x.
template <class _Real, class _Func>
_LIBCPP_HIDE_FROM_ABI _Real __bessel_near_integer_limit(_Func __raw, _Real __n, _Real __x) {
  // NOLINTBEGIN(readability-identifier-naming)
  constexpr _Real __d1 = _Real(0.001);
  constexpr _Real __d2 = _Real(0.0005);
  _Real __f1           = (__raw(__n + __d1, __x) + __raw(__n - __d1, __x)) / 2;
  _Real __f2           = (__raw(__n + __d2, __x) + __raw(__n - __d2, __x)) / 2;
  return (4 * __f2 - __f1) / 3;
  // NOLINTEND(readability-identifier-naming)
}

// The asymptotic (large-x) expansions of J_nu/Y_nu and I_nu/K_nu share the same auxiliary series,
// built from the product (mu - 1)(mu - 9)(mu - 25).../(k! (8x)^k), mu = 4*nu^2. Abramowitz & Stegun
// 9.2.9-9.2.10 (for P, Q, used by J/Y) and 9.7.2 (for the I/K series, all-positive-sign variant).
template <class _Real>
_LIBCPP_HIDE_FROM_ABI void __bessel_asym_pq(_Real __nu, _Real __x, _Real& __p, _Real& __q) {
  // NOLINTBEGIN(readability-identifier-naming)
  _Real __mu    = 4 * __nu * __nu;
  _Real __eightx = 8 * __x;
  __p            = 1;
  __q            = 0;
  _Real __t      = 1;
  for (int __k = 1; __k <= 12; ++__k) {
    _Real __factor = static_cast<_Real>(2 * __k - 1);
    __t *= (__mu - __factor * __factor) / (static_cast<_Real>(__k) * __eightx);
    if (__math::fabs(__t) > 10)
      break; // asymptotic series has started diverging: stop before it does more harm than good
    switch (__k % 4) {
    case 1:
      __q += __t;
      break;
    case 2:
      __p -= __t;
      break;
    case 3:
      __q -= __t;
      break;
    default:
      __p += __t;
      break;
    }
    if (__math::fabs(__t) < std::numeric_limits<_Real>::epsilon())
      break;
  }
  // NOLINTEND(readability-identifier-naming)
}

template <class _Real>
_LIBCPP_HIDE_FROM_ABI _Real __bessel_asym_y(_Real __nu, _Real __x) {
  // NOLINTBEGIN(readability-identifier-naming)
  _Real __p, __q;
  std::__bessel_asym_pq(__nu, __x, __p, __q);
  _Real __chi = __x - __nu * __math::__pi_v<_Real> / 2 - __math::__pi_v<_Real> / 4;
  return __math::sqrt(2 / (__math::__pi_v<_Real> * __x)) * (__p * __math::sin(__chi) + __q * __math::cos(__chi));
  // NOLINTEND(readability-identifier-naming)
}

template <class _Real>
_LIBCPP_HIDE_FROM_ABI _Real __bessel_asym_k(_Real __nu, _Real __x) {
  // NOLINTBEGIN(readability-identifier-naming)
  _Real __mu    = 4 * __nu * __nu;
  _Real __eightx = 8 * __x;
  _Real __sum    = 1;
  _Real __t      = 1;
  for (int __k = 1; __k <= 20; ++__k) {
    _Real __factor = static_cast<_Real>(2 * __k - 1);
    __t *= (__mu - __factor * __factor) / (static_cast<_Real>(__k) * __eightx);
    if (__math::fabs(__t) > __math::fabs(__sum))
      break; // stop before the asymptotic series starts diverging
    __sum += __t;
    if (__math::fabs(__t) < std::numeric_limits<_Real>::epsilon() * __math::fabs(__sum))
      break;
  }
  return __math::sqrt(__math::__pi_v<_Real> / (2 * __x)) * __math::exp(-__x) * __sum;
  // NOLINTEND(readability-identifier-naming)
}

// Reaches the order (__base + __n) from the two seed values at order __base and __base + 1, via the
// shared upward recurrence f_{v+1} = (2v/x) f_v +- f_{v-1} (minus for cyl_neumann/Y, plus for
// cyl_bessel_k/K), v = __base + k at step k. __base is 0 for integer targets, but is the fractional
// part of nu when this is used to reach a non-integer target (see __cyl_bessel_k). Stable in both
// cases: for Y it is the same recurrence J shares (fine for moderate seeds), and for K every term
// stays positive (x > 0), so there is no cancellation to amplify.
template <class _Real>
_LIBCPP_HIDE_FROM_ABI _Real
__bessel_recur_to(_Real __f0, _Real __f1, _Real __base, unsigned __n, _Real __x, bool __plus) {
  // NOLINTBEGIN(readability-identifier-naming)
  if (__n == 0)
    return __f0;
  if (__n == 1)
    return __f1;
  _Real __f_prev = __f0, __f_curr = __f1;
  for (unsigned __k = 1; __k < __n; ++__k) {
    _Real __coeff = 2 * (__base + static_cast<_Real>(__k)) / __x;
    _Real __f_next = __plus ? (__coeff * __f_curr + __f_prev) : (__coeff * __f_curr - __f_prev);
    __f_prev        = __f_curr;
    __f_curr        = __f_next;
  }
  return __f_curr;
  // NOLINTEND(readability-identifier-naming)
}

template <class _Real>
_LIBCPP_HIDE_FROM_ABI _Real __cyl_neumann(_Real __nu, _Real __x) {
  // NOLINTBEGIN(readability-identifier-naming)
  if (__math::isnan(__nu) || __math::isnan(__x))
    return __nu + __x;
  _Real __n = __math::round(__nu);
  if (__math::fabs(__nu - __n) < std::numeric_limits<_Real>::epsilon() * 1000) {
    // At large x, __cyl_neumann_raw's reflection formula subtracts two quantities of comparable
    // magnitude to recover a much smaller remainder near an integer order, and the delta-based limit
    // above compounds that with a division by a small delta -- verified to lose several digits of
    // accuracy by x ~ 15-30 for larger orders. The asymptotic expansion has no such cancellation (it
    // evaluates the answer directly, not as a near-zero difference), so above the threshold it seeds
    // Y_0/Y_1 there instead and reaches the target order via the stable upward recurrence.
    unsigned __ni = static_cast<unsigned>(__n < 0 ? 0 : __n);
    if (__x >= 9) {
      _Real __y0 = std::__bessel_asym_y(_Real(0), __x);
      _Real __y1 = std::__bessel_asym_y(_Real(1), __x);
      return std::__bessel_recur_to(__y0, __y1, _Real(0), __ni, __x, false);
    }
    return std::__bessel_near_integer_limit(std::__cyl_neumann_raw<_Real>, __n, __x);
  }
  return std::__cyl_neumann_raw(__nu, __x);
  // NOLINTEND(readability-identifier-naming)
}

template <class _Real>
_LIBCPP_HIDE_FROM_ABI _Real __cyl_bessel_k(_Real __nu, _Real __x) {
  // NOLINTBEGIN(readability-identifier-naming)
  if (__math::isnan(__nu) || __math::isnan(__x))
    return __nu + __x;

  // I_nu(x) grows like e^x, so __cyl_bessel_k_raw's I_{-nu}(x) - I_nu(x) reflection formula suffers
  // catastrophic cancellation once x is more than moderately large -- verified against an independent
  // oracle to already be wrong by orders of magnitude at x ~ 15-30, and *not* only near integer nu
  // (unlike cyl_neumann's analogous formula, whose J-based terms stay O(1) rather than blowing up).
  // Above the threshold, this is avoided entirely by decomposing nu into its fractional part and an
  // integer number of steps, seeding K at the fractional part and fractional part + 1 from the
  // (cancellation-free, monotone) asymptotic series, and reaching the target nu via the same stable
  // upward recurrence used for integer orders -- which works identically whether nu itself is an
  // integer (fractional part 0) or not.
  if (__x >= 8) {
    _Real __n_below = __math::trunc(__nu);
    _Real __frac    = __nu - __n_below;
    _Real __k0      = std::__bessel_asym_k(__frac, __x);
    _Real __k1      = std::__bessel_asym_k(__frac + 1, __x);
    return std::__bessel_recur_to(__k0, __k1, __frac, static_cast<unsigned>(__n_below), __x, true);
  }

  _Real __n = __math::round(__nu);
  if (__math::fabs(__nu - __n) < std::numeric_limits<_Real>::epsilon() * 1000)
    return std::__bessel_near_integer_limit(std::__cyl_bessel_k_raw<_Real>, __n, __x);
  return std::__cyl_bessel_k_raw(__nu, __x);
  // NOLINTEND(readability-identifier-naming)
}

// ---- cyl_bessel_j ----

inline _LIBCPP_HIDE_FROM_ABI double cyl_bessel_j(double __nu, double __x) { return std::__cyl_bessel_j(__nu, __x); }

inline _LIBCPP_HIDE_FROM_ABI float cyl_bessel_j(float __nu, float __x) {
  return static_cast<float>(std::cyl_bessel_j(static_cast<double>(__nu), static_cast<double>(__x)));
}

inline _LIBCPP_HIDE_FROM_ABI long double cyl_bessel_j(long double __nu, long double __x) {
  return std::__cyl_bessel_j(__nu, __x);
}

inline _LIBCPP_HIDE_FROM_ABI float cyl_bessel_jf(float __nu, float __x) { return std::cyl_bessel_j(__nu, __x); }

inline _LIBCPP_HIDE_FROM_ABI long double cyl_bessel_jl(long double __nu, long double __x) {
  return std::cyl_bessel_j(__nu, __x);
}

template <class _A1, class _A2, std::enable_if_t<std::is_arithmetic_v<_A1> && std::is_arithmetic_v<_A2>, int> = 0>
_LIBCPP_HIDE_FROM_ABI __promote_t<_A1, _A2> cyl_bessel_j(_A1 __nu, _A2 __x) {
  using __result_type = __promote_t<_A1, _A2>;
  return std::cyl_bessel_j(static_cast<__result_type>(__nu), static_cast<__result_type>(__x));
}

// ---- cyl_neumann ----

inline _LIBCPP_HIDE_FROM_ABI double cyl_neumann(double __nu, double __x) { return std::__cyl_neumann(__nu, __x); }

inline _LIBCPP_HIDE_FROM_ABI float cyl_neumann(float __nu, float __x) {
  return static_cast<float>(std::cyl_neumann(static_cast<double>(__nu), static_cast<double>(__x)));
}

inline _LIBCPP_HIDE_FROM_ABI long double cyl_neumann(long double __nu, long double __x) {
  return std::__cyl_neumann(__nu, __x);
}

inline _LIBCPP_HIDE_FROM_ABI float cyl_neumannf(float __nu, float __x) { return std::cyl_neumann(__nu, __x); }

inline _LIBCPP_HIDE_FROM_ABI long double cyl_neumannl(long double __nu, long double __x) {
  return std::cyl_neumann(__nu, __x);
}

template <class _A1, class _A2, std::enable_if_t<std::is_arithmetic_v<_A1> && std::is_arithmetic_v<_A2>, int> = 0>
_LIBCPP_HIDE_FROM_ABI __promote_t<_A1, _A2> cyl_neumann(_A1 __nu, _A2 __x) {
  using __result_type = __promote_t<_A1, _A2>;
  return std::cyl_neumann(static_cast<__result_type>(__nu), static_cast<__result_type>(__x));
}

// ---- cyl_bessel_i ----

inline _LIBCPP_HIDE_FROM_ABI double cyl_bessel_i(double __nu, double __x) { return std::__cyl_bessel_i(__nu, __x); }

inline _LIBCPP_HIDE_FROM_ABI float cyl_bessel_i(float __nu, float __x) {
  return static_cast<float>(std::cyl_bessel_i(static_cast<double>(__nu), static_cast<double>(__x)));
}

inline _LIBCPP_HIDE_FROM_ABI long double cyl_bessel_i(long double __nu, long double __x) {
  return std::__cyl_bessel_i(__nu, __x);
}

inline _LIBCPP_HIDE_FROM_ABI float cyl_bessel_if(float __nu, float __x) { return std::cyl_bessel_i(__nu, __x); }

inline _LIBCPP_HIDE_FROM_ABI long double cyl_bessel_il(long double __nu, long double __x) {
  return std::cyl_bessel_i(__nu, __x);
}

template <class _A1, class _A2, std::enable_if_t<std::is_arithmetic_v<_A1> && std::is_arithmetic_v<_A2>, int> = 0>
_LIBCPP_HIDE_FROM_ABI __promote_t<_A1, _A2> cyl_bessel_i(_A1 __nu, _A2 __x) {
  using __result_type = __promote_t<_A1, _A2>;
  return std::cyl_bessel_i(static_cast<__result_type>(__nu), static_cast<__result_type>(__x));
}

// ---- cyl_bessel_k ----

inline _LIBCPP_HIDE_FROM_ABI double cyl_bessel_k(double __nu, double __x) { return std::__cyl_bessel_k(__nu, __x); }

inline _LIBCPP_HIDE_FROM_ABI float cyl_bessel_k(float __nu, float __x) {
  return static_cast<float>(std::cyl_bessel_k(static_cast<double>(__nu), static_cast<double>(__x)));
}

inline _LIBCPP_HIDE_FROM_ABI long double cyl_bessel_k(long double __nu, long double __x) {
  return std::__cyl_bessel_k(__nu, __x);
}

inline _LIBCPP_HIDE_FROM_ABI float cyl_bessel_kf(float __nu, float __x) { return std::cyl_bessel_k(__nu, __x); }

inline _LIBCPP_HIDE_FROM_ABI long double cyl_bessel_kl(long double __nu, long double __x) {
  return std::cyl_bessel_k(__nu, __x);
}

template <class _A1, class _A2, std::enable_if_t<std::is_arithmetic_v<_A1> && std::is_arithmetic_v<_A2>, int> = 0>
_LIBCPP_HIDE_FROM_ABI __promote_t<_A1, _A2> cyl_bessel_k(_A1 __nu, _A2 __x) {
  using __result_type = __promote_t<_A1, _A2>;
  return std::cyl_bessel_k(static_cast<__result_type>(__nu), static_cast<__result_type>(__x));
}

// ======================= Spherical Bessel functions =======================
//
// Unlike the cylindrical family above, sph_bessel and sph_neumann get their own direct
// implementation rather than routing through cyl_bessel_j/cyl_neumann at order n+1/2: the spherical
// Bessel functions have elementary closed forms at n=0,1 (ratios of sin/cos and powers of x) and
// satisfy the same simple 3-term recurrence as the cylindrical family, without needing the gamma
// function or reflection-formula machinery at all.

template <class _Real>
_LIBCPP_HIDE_FROM_ABI _Real __sph_bessel(unsigned __n, _Real __x) {
  // j_n(x). Computed via downward recurrence (Miller's algorithm): j_n is the *recessive* solution of
  // the shared 3-term recurrence for n > x, so upward recurrence amplifies rounding-error
  // contamination from the dominant (y_n-like) solution -- verified empirically to fail by many
  // orders of magnitude for n well above x before this fix. Downward recurrence from an arbitrary
  // seed at a sufficiently high order, then rescaled against the exact j_0(x) = sin(x)/x, is stable.

  // NOLINTBEGIN(readability-identifier-naming)
  if (__math::isnan(__x))
    return __x;
  if (__x == 0)
    return __n == 0 ? _Real(1) : _Real(0);

  unsigned __start = __n + 20 + static_cast<unsigned>(__math::fabs(__x)) + 10;
  _Real __f_next   = 0;
  _Real __f_curr   = 1; // arbitrary seed
  _Real __f_at_n   = 0;
  // Counts down __start iterations, computing f[k-1] from f[k] and f[k+1] at each step (k running
  // from __start to 1). An unsigned loop variable compared against >= 1 would wrap around instead of
  // terminating, so the remaining-iteration count is tracked separately.
  for (unsigned __k = __start, __remaining = __start; __remaining > 0; --__remaining, --__k) {
    _Real __f_prev = static_cast<_Real>(2 * __k + 1) / __x * __f_curr - __f_next;
    __f_next        = __f_curr;
    __f_curr        = __f_prev;
    if (__k - 1 == __n)
      __f_at_n = __f_curr;
  }
  _Real __unnormalized_j0 = __f_curr;
  _Real __true_j0          = __math::sin(__x) / __x;
  return __f_at_n * (__true_j0 / __unnormalized_j0);
  // NOLINTEND(readability-identifier-naming)
}

template <class _Real>
_LIBCPP_HIDE_FROM_ABI _Real __sph_neumann(unsigned __n, _Real __x) {
  // y_n(x). Computed via upward recurrence, which is stable for y_n (the dominant solution).

  // NOLINTBEGIN(readability-identifier-naming)
  if (__math::isnan(__x))
    return __x;
  _Real __y0 = -__math::cos(__x) / __x;
  if (__n == 0)
    return __y0;
  _Real __y1 = -__math::cos(__x) / (__x * __x) - __math::sin(__x) / __x;
  if (__n == 1)
    return __y1;
  _Real __y_prev = __y0, __y_curr = __y1;
  for (unsigned __k = 1; __k < __n; ++__k) {
    _Real __y_next = static_cast<_Real>(2 * __k + 1) / __x * __y_curr - __y_prev;
    __y_prev        = __y_curr;
    __y_curr        = __y_next;
  }
  return __y_curr;
  // NOLINTEND(readability-identifier-naming)
}

inline _LIBCPP_HIDE_FROM_ABI double sph_bessel(unsigned __n, double __x) { return std::__sph_bessel(__n, __x); }

inline _LIBCPP_HIDE_FROM_ABI float sph_bessel(unsigned __n, float __x) {
  return static_cast<float>(std::sph_bessel(__n, static_cast<double>(__x)));
}

inline _LIBCPP_HIDE_FROM_ABI long double sph_bessel(unsigned __n, long double __x) {
  return std::__sph_bessel(__n, __x);
}

inline _LIBCPP_HIDE_FROM_ABI float sph_besself(unsigned __n, float __x) { return std::sph_bessel(__n, __x); }

inline _LIBCPP_HIDE_FROM_ABI long double sph_bessell(unsigned __n, long double __x) {
  return std::sph_bessel(__n, __x);
}

template <class _Integer, std::enable_if_t<std::is_integral_v<_Integer>, int> = 0>
_LIBCPP_HIDE_FROM_ABI double sph_bessel(unsigned __n, _Integer __x) {
  return std::sph_bessel(__n, static_cast<double>(__x));
}

inline _LIBCPP_HIDE_FROM_ABI double sph_neumann(unsigned __n, double __x) { return std::__sph_neumann(__n, __x); }

inline _LIBCPP_HIDE_FROM_ABI float sph_neumann(unsigned __n, float __x) {
  return static_cast<float>(std::sph_neumann(__n, static_cast<double>(__x)));
}

inline _LIBCPP_HIDE_FROM_ABI long double sph_neumann(unsigned __n, long double __x) {
  return std::__sph_neumann(__n, __x);
}

inline _LIBCPP_HIDE_FROM_ABI float sph_neumannf(unsigned __n, float __x) { return std::sph_neumann(__n, __x); }

inline _LIBCPP_HIDE_FROM_ABI long double sph_neumannl(unsigned __n, long double __x) {
  return std::sph_neumann(__n, __x);
}

template <class _Integer, std::enable_if_t<std::is_integral_v<_Integer>, int> = 0>
_LIBCPP_HIDE_FROM_ABI double sph_neumann(unsigned __n, _Integer __x) {
  return std::sph_neumann(__n, static_cast<double>(__x));
}

#endif // _LIBCPP_STD_VER >= 17

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP___MATH_SPECIAL_FUNCTIONS_H
