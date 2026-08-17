// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___SIMD_COMPLEX_H
#define _LIBCPP___SIMD_COMPLEX_H

#include <__config>
#include <__simd/abi.h>
#include <__simd/basic_vec.h>
#include <__simd/concepts.h>
#include <__simd/traits.h>
#include <complex>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

namespace simd {

// [simd.complex.math], P2663
//
// basic_vec's real-type member, its complex constructor, and its four complex accessors already
// live in basic_vec.h ([simd.overview]/3, [simd.ctor]/20-21, [simd.complex.access]). What remains
// here is the free-function surface: 25 names operating lane-wise via the corresponding <complex>
// scalar function.
//
// Same KNOWN DEVIATION as math.h: the clause declares each function with every V/deduced-vec-t<V>
// parameter combination, which is expression-template infrastructure that collapses to one
// signature per function for a basic_vec whose operator+ returns basic_vec<T,Abi> exactly. See the
// long-form rationale at the top of math.h; it applies verbatim to pow below.

// --- accessors returning the real-type vec: real, imag are noexcept; abs, arg, norm are not (5) ---
#  define _LIBCPP_SIMD_COMPLEX_TO_REAL(_Name)                                                                        \
    template <__simd_complex _Vp>                                                                                    \
    _LIBCPP_HIDE_FROM_ABI constexpr rebind_t<__simd_complex_value_type<_Vp>, _Vp> _Name(const _Vp& __x) {            \
      using _Rp = rebind_t<__simd_complex_value_type<_Vp>, _Vp>;                                                     \
      return _Rp([&](auto __i) { return std::_Name(__x[__i]); });                                                    \
    }

#  define _LIBCPP_SIMD_COMPLEX_TO_REAL_NOEXCEPT(_Name)                                                               \
    template <__simd_complex _Vp>                                                                                    \
    _LIBCPP_HIDE_FROM_ABI constexpr rebind_t<__simd_complex_value_type<_Vp>, _Vp> _Name(const _Vp& __x) noexcept {   \
      using _Rp = rebind_t<__simd_complex_value_type<_Vp>, _Vp>;                                                     \
      return _Rp([&](auto __i) { return std::_Name(__x[__i]); });                                                    \
    }

_LIBCPP_SIMD_COMPLEX_TO_REAL_NOEXCEPT(real)
_LIBCPP_SIMD_COMPLEX_TO_REAL_NOEXCEPT(imag)
_LIBCPP_SIMD_COMPLEX_TO_REAL(abs)
_LIBCPP_SIMD_COMPLEX_TO_REAL(arg)
_LIBCPP_SIMD_COMPLEX_TO_REAL(norm)

#  undef _LIBCPP_SIMD_COMPLEX_TO_REAL
#  undef _LIBCPP_SIMD_COMPLEX_TO_REAL_NOEXCEPT

// --- complex-valued transcendental/trigonometric/hyperbolic functions, all constexpr (18) ---
#  define _LIBCPP_SIMD_COMPLEX_TO_COMPLEX(_Name)                                                                     \
    template <__simd_complex _Vp>                                                                                    \
    _LIBCPP_HIDE_FROM_ABI constexpr _Vp _Name(const _Vp& __x) {                                                      \
      return _Vp([&](auto __i) { return std::_Name(__x[__i]); });                                                    \
    }

_LIBCPP_SIMD_COMPLEX_TO_COMPLEX(conj)
_LIBCPP_SIMD_COMPLEX_TO_COMPLEX(proj)
_LIBCPP_SIMD_COMPLEX_TO_COMPLEX(exp)
_LIBCPP_SIMD_COMPLEX_TO_COMPLEX(log)
_LIBCPP_SIMD_COMPLEX_TO_COMPLEX(log10)
_LIBCPP_SIMD_COMPLEX_TO_COMPLEX(sqrt)
_LIBCPP_SIMD_COMPLEX_TO_COMPLEX(sin)
_LIBCPP_SIMD_COMPLEX_TO_COMPLEX(asin)
_LIBCPP_SIMD_COMPLEX_TO_COMPLEX(cos)
_LIBCPP_SIMD_COMPLEX_TO_COMPLEX(acos)
_LIBCPP_SIMD_COMPLEX_TO_COMPLEX(tan)
_LIBCPP_SIMD_COMPLEX_TO_COMPLEX(atan)
_LIBCPP_SIMD_COMPLEX_TO_COMPLEX(sinh)
_LIBCPP_SIMD_COMPLEX_TO_COMPLEX(asinh)
_LIBCPP_SIMD_COMPLEX_TO_COMPLEX(cosh)
_LIBCPP_SIMD_COMPLEX_TO_COMPLEX(acosh)
_LIBCPP_SIMD_COMPLEX_TO_COMPLEX(tanh)
_LIBCPP_SIMD_COMPLEX_TO_COMPLEX(atanh)

#  undef _LIBCPP_SIMD_COMPLEX_TO_COMPLEX

// --- pow(x, y): homogeneous simd-complex/simd-complex, constexpr (1) ---
template <__simd_complex _Vp>
_LIBCPP_HIDE_FROM_ABI constexpr _Vp pow(const _Vp& __x, const _Vp& __y) {
  return _Vp([&](auto __i) { return std::pow(__x[__i], __y[__i]); });
}

// --- polar(rho, theta): the one function taking simd-floating-point (not simd-complex) operands,
// and the one the clause marks *not* constexpr (1) ---
template <__simd_floating_point _Vp>
_LIBCPP_HIDE_FROM_ABI rebind_t<complex<typename _Vp::value_type>, _Vp> polar(const _Vp& __x, const _Vp& __y = _Vp()) {
  using _Rp = rebind_t<complex<typename _Vp::value_type>, _Vp>;
  return _Rp([&](auto __i) { return std::polar(__x[__i], __y[__i]); });
}

} // namespace simd

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___SIMD_COMPLEX_H
