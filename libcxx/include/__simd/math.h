// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___SIMD_MATH_H
#define _LIBCPP___SIMD_MATH_H

#include <__concepts/arithmetic.h>
#include <__config>
#include <__simd/abi.h>
#include <__simd/basic_vec.h>
#include <__simd/concepts.h>
#include <__simd/traits.h>
#include <__type_traits/type_identity.h>
#include <cmath>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

namespace simd {

// [simd.math]
//
// KNOWN DEVIATION, documented rather than silently taken: the clause declares each multi-argument
// function with every combination of `const V&` and `const deduced-vec-t<V>&` parameters (up to 7
// overloads for the 3-argument functions). That surface exists to support *expression-template*
// implementations, where operator+ etc. return a lazy proxy type distinct from basic_vec and
// deduced-vec-t<V> is the mechanism that forces materialization to a concrete vector.
//
// This implementation's basic_vec::operator+ returns basic_vec<T, Abi> by value, exactly and always
// -- there is no proxy type. Consequently, for any V that is actually a basic_vec specialization,
// __deduced_vec_t<V> is *the same type* as V (decltype(x + x) for x : const V& is V itself). Under
// that identity, the declared "combinations" are not different functions: they are the same
// function template specialization spelled several times, which would be an outright redefinition,
// not merely redundant. The single signature below is therefore the entire conforming surface for
// a non-expression-template implementation, and every valid call under the full clause (including
// scalar-vs-vec mixes, since scalars reach V via basic_vec's broadcasting constructor either way)
// resolves identically through it.
template <__math_floating_point _Vp>
using __math_result_t = __deduced_vec_t<_Vp>;

#  define _LIBCPP_SIMD_MATH_UNARY(_Name)                                                                             \
    template <__math_floating_point _Vp>                                                                             \
    _LIBCPP_HIDE_FROM_ABI constexpr __math_result_t<_Vp> _Name(const _Vp& __x) {                                     \
      using _Rp = __math_result_t<_Vp>;                                                                              \
      return _Rp([&](auto __i) { return std::_Name(__x[__i]); });                                                   \
    }

#  define _LIBCPP_SIMD_MATH_UNARY_NONCONSTEXPR(_Name)                                                                \
    template <__math_floating_point _Vp>                                                                             \
    _LIBCPP_HIDE_FROM_ABI __math_result_t<_Vp> _Name(const _Vp& __x) {                                               \
      using _Rp = __math_result_t<_Vp>;                                                                              \
      return _Rp([&](auto __i) { return std::_Name(__x[__i]); });                                                   \
    }

#  define _LIBCPP_SIMD_MATH_BINARY(_Name)                                                                            \
    template <__math_floating_point _Vp>                                                                             \
    _LIBCPP_HIDE_FROM_ABI constexpr __math_result_t<_Vp> _Name(const _Vp& __x, const _Vp& __y) {                     \
      using _Rp = __math_result_t<_Vp>;                                                                              \
      return _Rp([&](auto __i) { return std::_Name(__x[__i], __y[__i]); });                                         \
    }

#  define _LIBCPP_SIMD_MATH_TERNARY(_Name)                                                                           \
    template <__math_floating_point _Vp>                                                                             \
    _LIBCPP_HIDE_FROM_ABI constexpr __math_result_t<_Vp> _Name(const _Vp& __x, const _Vp& __y, const _Vp& __z) {     \
      using _Rp = __math_result_t<_Vp>;                                                                              \
      return _Rp([&](auto __i) { return std::_Name(__x[__i], __y[__i], __z[__i]); });                                \
    }

// Classification functions returning a mask, [simd.math]: isfinite/isinf/.../signbit (unary),
// isgreater/.../isunordered (binary).
#  define _LIBCPP_SIMD_MATH_UNARY_MASK(_Name)                                                                        \
    template <__math_floating_point _Vp>                                                                             \
    _LIBCPP_HIDE_FROM_ABI constexpr typename __math_result_t<_Vp>::mask_type _Name(const _Vp& __x) {                \
      using _Mp = typename __math_result_t<_Vp>::mask_type;                                                         \
      return _Mp([&](auto __i) -> bool { return std::_Name(__x[__i]); });                                           \
    }

#  define _LIBCPP_SIMD_MATH_BINARY_MASK(_Name)                                                                       \
    template <__math_floating_point _Vp>                                                                             \
    _LIBCPP_HIDE_FROM_ABI constexpr typename __math_result_t<_Vp>::mask_type _Name(const _Vp& __x, const _Vp& __y) { \
      using _Mp = typename __math_result_t<_Vp>::mask_type;                                                         \
      return _Mp([&](auto __i) -> bool { return std::_Name(__x[__i], __y[__i]); });                                 \
    }

// --- trigonometric, hyperbolic, exponential, logarithmic, rounding (27) ---
_LIBCPP_SIMD_MATH_UNARY(acos)
_LIBCPP_SIMD_MATH_UNARY(asin)
_LIBCPP_SIMD_MATH_UNARY(atan)
_LIBCPP_SIMD_MATH_UNARY(cos)
_LIBCPP_SIMD_MATH_UNARY(sin)
_LIBCPP_SIMD_MATH_UNARY(tan)
_LIBCPP_SIMD_MATH_UNARY(acosh)
_LIBCPP_SIMD_MATH_UNARY(asinh)
_LIBCPP_SIMD_MATH_UNARY(atanh)
_LIBCPP_SIMD_MATH_UNARY(cosh)
_LIBCPP_SIMD_MATH_UNARY(sinh)
_LIBCPP_SIMD_MATH_UNARY(tanh)
_LIBCPP_SIMD_MATH_UNARY(exp)
_LIBCPP_SIMD_MATH_UNARY(exp2)
_LIBCPP_SIMD_MATH_UNARY(expm1)
_LIBCPP_SIMD_MATH_UNARY(log)
_LIBCPP_SIMD_MATH_UNARY(log10)
_LIBCPP_SIMD_MATH_UNARY(log1p)
_LIBCPP_SIMD_MATH_UNARY(log2)
_LIBCPP_SIMD_MATH_UNARY(logb)
_LIBCPP_SIMD_MATH_UNARY(cbrt)
_LIBCPP_SIMD_MATH_UNARY(sqrt)
_LIBCPP_SIMD_MATH_UNARY(erf)
_LIBCPP_SIMD_MATH_UNARY(erfc)
_LIBCPP_SIMD_MATH_UNARY(lgamma)
_LIBCPP_SIMD_MATH_UNARY(tgamma)
_LIBCPP_SIMD_MATH_UNARY(ceil)
_LIBCPP_SIMD_MATH_UNARY(floor)
_LIBCPP_SIMD_MATH_UNARY(trunc)
_LIBCPP_SIMD_MATH_UNARY(fabs)
_LIBCPP_SIMD_MATH_UNARY(round)

// --- 2-argument (10) ---
_LIBCPP_SIMD_MATH_BINARY(atan2)
_LIBCPP_SIMD_MATH_BINARY(hypot)
_LIBCPP_SIMD_MATH_BINARY(pow)
_LIBCPP_SIMD_MATH_BINARY(fmod)
_LIBCPP_SIMD_MATH_BINARY(remainder)
_LIBCPP_SIMD_MATH_BINARY(copysign)
_LIBCPP_SIMD_MATH_BINARY(nextafter)
_LIBCPP_SIMD_MATH_BINARY(fdim)
_LIBCPP_SIMD_MATH_BINARY(fmax)
_LIBCPP_SIMD_MATH_BINARY(fmin)

// --- classification, unary -> mask (5) ---
_LIBCPP_SIMD_MATH_UNARY_MASK(isfinite)
_LIBCPP_SIMD_MATH_UNARY_MASK(isinf)
_LIBCPP_SIMD_MATH_UNARY_MASK(isnan)
_LIBCPP_SIMD_MATH_UNARY_MASK(isnormal)
_LIBCPP_SIMD_MATH_UNARY_MASK(signbit)

// --- classification, binary -> mask (6) ---
_LIBCPP_SIMD_MATH_BINARY_MASK(isgreater)
_LIBCPP_SIMD_MATH_BINARY_MASK(isgreaterequal)
_LIBCPP_SIMD_MATH_BINARY_MASK(isless)
_LIBCPP_SIMD_MATH_BINARY_MASK(islessequal)
_LIBCPP_SIMD_MATH_BINARY_MASK(islessgreater)
_LIBCPP_SIMD_MATH_BINARY_MASK(isunordered)

// --- non-constexpr per [simd.math]: nearbyint, rint, lrint, llrint (4) ---
_LIBCPP_SIMD_MATH_UNARY_NONCONSTEXPR(nearbyint)
_LIBCPP_SIMD_MATH_UNARY_NONCONSTEXPR(rint)

template <__math_floating_point _Vp>
_LIBCPP_HIDE_FROM_ABI rebind_t<long int, __math_result_t<_Vp>> lrint(const _Vp& __x) {
  using _Rp = rebind_t<long int, __math_result_t<_Vp>>;
  return _Rp([&](auto __i) { return std::lrint(__x[__i]); });
}

template <__math_floating_point _Vp>
_LIBCPP_HIDE_FROM_ABI rebind_t<long long int, __math_result_t<_Vp>> llrint(const _Vp& __x) {
  using _Rp = rebind_t<long long int, __math_result_t<_Vp>>;
  return _Rp([&](auto __i) { return std::llrint(__x[__i]); });
}

// --- rounding to integer types, constexpr (3) ---
template <__math_floating_point _Vp>
_LIBCPP_HIDE_FROM_ABI constexpr rebind_t<long int, __math_result_t<_Vp>> lround(const _Vp& __x) {
  using _Rp = rebind_t<long int, __math_result_t<_Vp>>;
  return _Rp([&](auto __i) { return std::lround(__x[__i]); });
}

template <__math_floating_point _Vp>
_LIBCPP_HIDE_FROM_ABI constexpr rebind_t<long long int, __math_result_t<_Vp>> llround(const _Vp& __x) {
  using _Rp = rebind_t<long long int, __math_result_t<_Vp>>;
  return _Rp([&](auto __i) { return std::llround(__x[__i]); });
}

// --- 3-argument (2) ---
_LIBCPP_SIMD_MATH_TERNARY(fma)
_LIBCPP_SIMD_MATH_TERNARY(hypot) // three-argument hypot overload, C++17

template <__math_floating_point _Vp>
_LIBCPP_HIDE_FROM_ABI constexpr __math_result_t<_Vp> lerp(const _Vp& __a, const _Vp& __b, const _Vp& __t) noexcept {
  using _Rp = __math_result_t<_Vp>;
  return _Rp([&](auto __i) { return std::lerp(__a[__i], __b[__i], __t[__i]); });
}

#  undef _LIBCPP_SIMD_MATH_UNARY
#  undef _LIBCPP_SIMD_MATH_UNARY_NONCONSTEXPR
#  undef _LIBCPP_SIMD_MATH_BINARY
#  undef _LIBCPP_SIMD_MATH_TERNARY
#  undef _LIBCPP_SIMD_MATH_UNARY_MASK
#  undef _LIBCPP_SIMD_MATH_BINARY_MASK

// --- abs: two distinct overloads, signed-integral and floating-point ---
template <class _Tp, class _Abi>
  requires(__vec_enabled<_Tp, _Abi> && signed_integral<_Tp>)
_LIBCPP_HIDE_FROM_ABI constexpr basic_vec<_Tp, _Abi> abs(const basic_vec<_Tp, _Abi>& __j) {
  return basic_vec<_Tp, _Abi>([&](auto __i) { return static_cast<_Tp>(std::abs(__j[__i])); });
}

template <__math_floating_point _Vp>
_LIBCPP_HIDE_FROM_ABI constexpr __math_result_t<_Vp> abs(const _Vp& __j) {
  using _Rp = __math_result_t<_Vp>;
  return _Rp([&](auto __i) { return std::abs(__j[__i]); });
}

// --- ilogb, fpclassify: unary -> int-width vec ---
template <__math_floating_point _Vp>
_LIBCPP_HIDE_FROM_ABI constexpr rebind_t<int, __math_result_t<_Vp>> ilogb(const _Vp& __x) {
  using _Rp = rebind_t<int, __math_result_t<_Vp>>;
  return _Rp([&](auto __i) { return std::ilogb(__x[__i]); });
}

template <__math_floating_point _Vp>
_LIBCPP_HIDE_FROM_ABI constexpr rebind_t<int, __math_result_t<_Vp>> fpclassify(const _Vp& __x) {
  using _Rp = rebind_t<int, __math_result_t<_Vp>>;
  return _Rp([&](auto __i) { return std::fpclassify(__x[__i]); });
}

// --- frexp(value, exp*), scalbn/scalbln(x, n), ldexp(x, exp): mixed exponent-vec signatures ---
template <__math_floating_point _Vp>
_LIBCPP_HIDE_FROM_ABI constexpr __math_result_t<_Vp> frexp(const _Vp& __value, rebind_t<int, __math_result_t<_Vp>>* __exp) {
  using _Rp = __math_result_t<_Vp>;
  using _Tp = typename _Rp::value_type;
  _Tp __mantissa[static_cast<size_t>(_Rp::size())];
  int __exponent[static_cast<size_t>(_Rp::size())];
  for (__simd_size_type __i = 0; __i != _Rp::size(); ++__i)
    __mantissa[__i] = std::frexp(__value[__i], &__exponent[__i]);
  *__exp = rebind_t<int, __math_result_t<_Vp>>([&](auto __i) { return __exponent[__i]; });
  return _Rp([&](auto __i) { return __mantissa[__i]; });
}

template <__math_floating_point _Vp>
_LIBCPP_HIDE_FROM_ABI constexpr __math_result_t<_Vp>
ldexp(const _Vp& __x, const rebind_t<int, __math_result_t<_Vp>>& __exp) {
  using _Rp = __math_result_t<_Vp>;
  return _Rp([&](auto __i) { return std::ldexp(__x[__i], __exp[__i]); });
}

template <__math_floating_point _Vp>
_LIBCPP_HIDE_FROM_ABI constexpr __math_result_t<_Vp>
scalbn(const _Vp& __x, const rebind_t<int, __math_result_t<_Vp>>& __n) {
  using _Rp = __math_result_t<_Vp>;
  return _Rp([&](auto __i) { return std::scalbn(__x[__i], __n[__i]); });
}

template <__math_floating_point _Vp>
_LIBCPP_HIDE_FROM_ABI constexpr __math_result_t<_Vp>
scalbln(const _Vp& __x, const rebind_t<long int, __math_result_t<_Vp>>& __n) {
  using _Rp = __math_result_t<_Vp>;
  return _Rp([&](auto __i) { return std::scalbln(__x[__i], __n[__i]); });
}

// --- modf(value, iptr*): [simd.math] deliberately makes `value` a non-deduced parameter
// (type_identity_t<basic_vec<T,Abi>>) so that T, Abi are pinned by iptr instead. ---
template <class _Tp, class _Abi>
  requires __vec_enabled<_Tp, _Abi>
_LIBCPP_HIDE_FROM_ABI constexpr basic_vec<_Tp, _Abi>
modf(const type_identity_t<basic_vec<_Tp, _Abi>>& __value, basic_vec<_Tp, _Abi>* __iptr) {
  using _Vp = basic_vec<_Tp, _Abi>;
  _Tp __int_part[static_cast<size_t>(_Vp::size())];
  _Tp __frac_part[static_cast<size_t>(_Vp::size())];
  for (__simd_size_type __i = 0; __i != _Vp::size(); ++__i)
    __frac_part[__i] = std::modf(__value[__i], &__int_part[__i]);
  *__iptr = _Vp([&](auto __i) { return __int_part[__i]; });
  return _Vp([&](auto __i) { return __frac_part[__i]; });
}

// --- remquo(x, y, quo*) ---
template <__math_floating_point _Vp>
_LIBCPP_HIDE_FROM_ABI constexpr __math_result_t<_Vp>
remquo(const _Vp& __x, const _Vp& __y, rebind_t<int, __math_result_t<_Vp>>* __quo) {
  using _Rp = __math_result_t<_Vp>;
  using _Tp = typename _Rp::value_type;
  _Tp __result[static_cast<size_t>(_Rp::size())];
  int __quotient[static_cast<size_t>(_Rp::size())];
  for (__simd_size_type __i = 0; __i != _Rp::size(); ++__i)
    __result[__i] = std::remquo(__x[__i], __y[__i], &__quotient[__i]);
  *__quo = rebind_t<int, __math_result_t<_Vp>>([&](auto __i) { return __quotient[__i]; });
  return _Rp([&](auto __i) { return __result[__i]; });
}

} // namespace simd

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___SIMD_MATH_H
