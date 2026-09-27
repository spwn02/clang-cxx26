//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___MATH_STDFLOAT_H
#define _LIBCPP___MATH_STDFLOAT_H

#include <__config>
#include <__type_traits/enable_if.h>
#include <__type_traits/is_same.h>
#include <__type_traits/promote.h>

#if defined(__GLIBC__)
#  include <features.h>
#endif

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_STD_VER >= 23
_LIBCPP_BEGIN_NAMESPACE_STD
namespace __math {

// The C math ABI provides float and double entry points. Narrow extended
// formats are evaluated through float; float64_t uses double. float128_t is
// intentionally limited below to builtins that preserve its precision.
template <class _Float>
inline constexpr bool __is_stdfloat_math_v =
#  if defined(__STDCPP_FLOAT16_T__)
    is_same<_Float, _Float16>::value ||
#  endif
#  if defined(__STDCPP_BFLOAT16_T__)
    is_same<_Float, __bf16>::value ||
#  endif
#  if defined(__STDCPP_FLOAT32_T__)
    is_same<_Float, __float32>::value ||
#  endif
#  if defined(__STDCPP_FLOAT64_T__)
    is_same<_Float, __float64>::value ||
#  endif
    false;

template <class _Float>
inline constexpr bool __is_stdfloat64_v =
#  if defined(__STDCPP_FLOAT64_T__)
    is_same<_Float, __float64>::value;
#  else
    false;
#  endif

template <class _Float, __enable_if_t<__is_stdfloat_math_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline _LIBCPP_CONSTEXPR_SINCE_CXX23 _Float acos(_Float __value) _NOEXCEPT {
  if constexpr (__is_stdfloat64_v<_Float>)
    return static_cast<_Float>(__builtin_acos(static_cast<double>(__value)));
  else
    return static_cast<_Float>(__builtin_acosf(static_cast<float>(__value)));
}

template <class _Float, __enable_if_t<__is_stdfloat_math_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline _LIBCPP_CONSTEXPR_SINCE_CXX23 _Float asin(_Float __value) _NOEXCEPT {
  if constexpr (__is_stdfloat64_v<_Float>)
    return static_cast<_Float>(__builtin_asin(static_cast<double>(__value)));
  else
    return static_cast<_Float>(__builtin_asinf(static_cast<float>(__value)));
}

template <class _Float, __enable_if_t<__is_stdfloat_math_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline _LIBCPP_CONSTEXPR_SINCE_CXX23 _Float atan(_Float __value) _NOEXCEPT {
  if constexpr (__is_stdfloat64_v<_Float>)
    return static_cast<_Float>(__builtin_atan(static_cast<double>(__value)));
  else
    return static_cast<_Float>(__builtin_atanf(static_cast<float>(__value)));
}

template <class _Float, __enable_if_t<__is_stdfloat_math_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline _LIBCPP_CONSTEXPR_SINCE_CXX23 _Float acosh(_Float __value) _NOEXCEPT {
  if constexpr (__is_stdfloat64_v<_Float>)
    return static_cast<_Float>(__builtin_acosh(static_cast<double>(__value)));
  else
    return static_cast<_Float>(__builtin_acoshf(static_cast<float>(__value)));
}

template <class _Float, __enable_if_t<__is_stdfloat_math_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline _LIBCPP_CONSTEXPR_SINCE_CXX23 _Float asinh(_Float __value) _NOEXCEPT {
  if constexpr (__is_stdfloat64_v<_Float>)
    return static_cast<_Float>(__builtin_asinh(static_cast<double>(__value)));
  else
    return static_cast<_Float>(__builtin_asinhf(static_cast<float>(__value)));
}

template <class _Float, __enable_if_t<__is_stdfloat_math_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline _LIBCPP_CONSTEXPR_SINCE_CXX23 _Float atanh(_Float __value) _NOEXCEPT {
  if constexpr (__is_stdfloat64_v<_Float>)
    return static_cast<_Float>(__builtin_atanh(static_cast<double>(__value)));
  else
    return static_cast<_Float>(__builtin_atanhf(static_cast<float>(__value)));
}

template <class _Float, __enable_if_t<__is_stdfloat_math_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline _LIBCPP_CONSTEXPR_SINCE_CXX23 _Float cos(_Float __value) _NOEXCEPT {
  if constexpr (__is_stdfloat64_v<_Float>)
    return static_cast<_Float>(__builtin_cos(static_cast<double>(__value)));
  else
    return static_cast<_Float>(__builtin_cosf(static_cast<float>(__value)));
}

template <class _Float, __enable_if_t<__is_stdfloat_math_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline _LIBCPP_CONSTEXPR_SINCE_CXX23 _Float cosh(_Float __value) _NOEXCEPT {
  if constexpr (__is_stdfloat64_v<_Float>)
    return static_cast<_Float>(__builtin_cosh(static_cast<double>(__value)));
  else
    return static_cast<_Float>(__builtin_coshf(static_cast<float>(__value)));
}

template <class _Float, __enable_if_t<__is_stdfloat_math_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline _LIBCPP_CONSTEXPR_SINCE_CXX23 _Float sin(_Float __value) _NOEXCEPT {
  if constexpr (__is_stdfloat64_v<_Float>)
    return static_cast<_Float>(__builtin_sin(static_cast<double>(__value)));
  else
    return static_cast<_Float>(__builtin_sinf(static_cast<float>(__value)));
}

template <class _Float, __enable_if_t<__is_stdfloat_math_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline _LIBCPP_CONSTEXPR_SINCE_CXX23 _Float sinh(_Float __value) _NOEXCEPT {
  if constexpr (__is_stdfloat64_v<_Float>)
    return static_cast<_Float>(__builtin_sinh(static_cast<double>(__value)));
  else
    return static_cast<_Float>(__builtin_sinhf(static_cast<float>(__value)));
}

template <class _Float, __enable_if_t<__is_stdfloat_math_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline _LIBCPP_CONSTEXPR_SINCE_CXX23 _Float tan(_Float __value) _NOEXCEPT {
  if constexpr (__is_stdfloat64_v<_Float>)
    return static_cast<_Float>(__builtin_tan(static_cast<double>(__value)));
  else
    return static_cast<_Float>(__builtin_tanf(static_cast<float>(__value)));
}

template <class _Float, __enable_if_t<__is_stdfloat_math_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline _LIBCPP_CONSTEXPR_SINCE_CXX23 _Float tanh(_Float __value) _NOEXCEPT {
  if constexpr (__is_stdfloat64_v<_Float>)
    return static_cast<_Float>(__builtin_tanh(static_cast<double>(__value)));
  else
    return static_cast<_Float>(__builtin_tanhf(static_cast<float>(__value)));
}

template <class _Float, __enable_if_t<__is_stdfloat_math_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline _LIBCPP_CONSTEXPR_SINCE_CXX23 _Float exp(_Float __value) _NOEXCEPT {
  if constexpr (__is_stdfloat64_v<_Float>)
    return static_cast<_Float>(__builtin_exp(static_cast<double>(__value)));
  else
    return static_cast<_Float>(__builtin_expf(static_cast<float>(__value)));
}

template <class _Float, __enable_if_t<__is_stdfloat_math_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline _LIBCPP_CONSTEXPR_SINCE_CXX23 _Float exp2(_Float __value) _NOEXCEPT {
  if constexpr (__is_stdfloat64_v<_Float>)
    return static_cast<_Float>(__builtin_exp2(static_cast<double>(__value)));
  else
    return static_cast<_Float>(__builtin_exp2f(static_cast<float>(__value)));
}

template <class _Float, __enable_if_t<__is_stdfloat_math_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline _LIBCPP_CONSTEXPR_SINCE_CXX23 _Float expm1(_Float __value) _NOEXCEPT {
  if constexpr (__is_stdfloat64_v<_Float>)
    return static_cast<_Float>(__builtin_expm1(static_cast<double>(__value)));
  else
    return static_cast<_Float>(__builtin_expm1f(static_cast<float>(__value)));
}

template <class _Float, __enable_if_t<__is_stdfloat_math_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline _LIBCPP_CONSTEXPR_SINCE_CXX23 _Float log(_Float __value) _NOEXCEPT {
  if constexpr (__is_stdfloat64_v<_Float>)
    return static_cast<_Float>(__builtin_log(static_cast<double>(__value)));
  else
    return static_cast<_Float>(__builtin_logf(static_cast<float>(__value)));
}

template <class _Float, __enable_if_t<__is_stdfloat_math_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline _LIBCPP_CONSTEXPR_SINCE_CXX23 _Float log10(_Float __value) _NOEXCEPT {
  if constexpr (__is_stdfloat64_v<_Float>)
    return static_cast<_Float>(__builtin_log10(static_cast<double>(__value)));
  else
    return static_cast<_Float>(__builtin_log10f(static_cast<float>(__value)));
}

template <class _Float, __enable_if_t<__is_stdfloat_math_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline _LIBCPP_CONSTEXPR_SINCE_CXX23 _Float log1p(_Float __value) _NOEXCEPT {
  if constexpr (__is_stdfloat64_v<_Float>)
    return static_cast<_Float>(__builtin_log1p(static_cast<double>(__value)));
  else
    return static_cast<_Float>(__builtin_log1pf(static_cast<float>(__value)));
}

template <class _Float, __enable_if_t<__is_stdfloat_math_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline _LIBCPP_CONSTEXPR_SINCE_CXX23 _Float log2(_Float __value) _NOEXCEPT {
  if constexpr (__is_stdfloat64_v<_Float>)
    return static_cast<_Float>(__builtin_log2(static_cast<double>(__value)));
  else
    return static_cast<_Float>(__builtin_log2f(static_cast<float>(__value)));
}

template <class _Float, __enable_if_t<__is_stdfloat_math_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline _LIBCPP_CONSTEXPR_SINCE_CXX23 _Float logb(_Float __value) _NOEXCEPT {
  if constexpr (__is_stdfloat64_v<_Float>)
    return static_cast<_Float>(__builtin_logb(static_cast<double>(__value)));
  else
    return static_cast<_Float>(__builtin_logbf(static_cast<float>(__value)));
}

template <class _Float, __enable_if_t<__is_stdfloat_math_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline _LIBCPP_CONSTEXPR_SINCE_CXX23 _Float cbrt(_Float __value) _NOEXCEPT {
  if constexpr (__is_stdfloat64_v<_Float>)
    return static_cast<_Float>(__builtin_cbrt(static_cast<double>(__value)));
  else
    return static_cast<_Float>(__builtin_cbrtf(static_cast<float>(__value)));
}

template <class _Float, __enable_if_t<__is_stdfloat_math_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline _LIBCPP_CONSTEXPR_SINCE_CXX23 _Float ceil(_Float __value) _NOEXCEPT {
  if constexpr (__is_stdfloat64_v<_Float>)
    return static_cast<_Float>(__builtin_ceil(static_cast<double>(__value)));
  else
    return static_cast<_Float>(__builtin_ceilf(static_cast<float>(__value)));
}

template <class _Float, __enable_if_t<__is_stdfloat_math_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline _LIBCPP_CONSTEXPR_SINCE_CXX23 _Float floor(_Float __value) _NOEXCEPT {
  if constexpr (__is_stdfloat64_v<_Float>)
    return static_cast<_Float>(__builtin_floor(static_cast<double>(__value)));
  else
    return static_cast<_Float>(__builtin_floorf(static_cast<float>(__value)));
}

template <class _Float, __enable_if_t<__is_stdfloat_math_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline _LIBCPP_CONSTEXPR_SINCE_CXX23 _Float nearbyint(_Float __value) _NOEXCEPT {
  if constexpr (__is_stdfloat64_v<_Float>)
    return static_cast<_Float>(__builtin_nearbyint(static_cast<double>(__value)));
  else
    return static_cast<_Float>(__builtin_nearbyintf(static_cast<float>(__value)));
}

template <class _Float, __enable_if_t<__is_stdfloat_math_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline _LIBCPP_CONSTEXPR_SINCE_CXX23 _Float rint(_Float __value) _NOEXCEPT {
  if constexpr (__is_stdfloat64_v<_Float>)
    return static_cast<_Float>(__builtin_rint(static_cast<double>(__value)));
  else
    return static_cast<_Float>(__builtin_rintf(static_cast<float>(__value)));
}

template <class _Float, __enable_if_t<__is_stdfloat_math_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline _LIBCPP_CONSTEXPR_SINCE_CXX23 _Float round(_Float __value) _NOEXCEPT {
  if constexpr (__is_stdfloat64_v<_Float>)
    return static_cast<_Float>(__builtin_round(static_cast<double>(__value)));
  else
    return static_cast<_Float>(__builtin_roundf(static_cast<float>(__value)));
}

template <class _Float, __enable_if_t<__is_stdfloat_math_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline _LIBCPP_CONSTEXPR_SINCE_CXX23 _Float trunc(_Float __value) _NOEXCEPT {
  if constexpr (__is_stdfloat64_v<_Float>)
    return static_cast<_Float>(__builtin_trunc(static_cast<double>(__value)));
  else
    return static_cast<_Float>(__builtin_truncf(static_cast<float>(__value)));
}

template <class _Float, __enable_if_t<__is_stdfloat_math_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline _LIBCPP_CONSTEXPR_SINCE_CXX23 _Float erf(_Float __value) _NOEXCEPT {
  if constexpr (__is_stdfloat64_v<_Float>)
    return static_cast<_Float>(__builtin_erf(static_cast<double>(__value)));
  else
    return static_cast<_Float>(__builtin_erff(static_cast<float>(__value)));
}

template <class _Float, __enable_if_t<__is_stdfloat_math_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline _LIBCPP_CONSTEXPR_SINCE_CXX23 _Float erfc(_Float __value) _NOEXCEPT {
  if constexpr (__is_stdfloat64_v<_Float>)
    return static_cast<_Float>(__builtin_erfc(static_cast<double>(__value)));
  else
    return static_cast<_Float>(__builtin_erfcf(static_cast<float>(__value)));
}

template <class _Float, __enable_if_t<__is_stdfloat_math_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline _LIBCPP_CONSTEXPR_SINCE_CXX23 _Float tgamma(_Float __value) _NOEXCEPT {
  if constexpr (__is_stdfloat64_v<_Float>)
    return static_cast<_Float>(__builtin_tgamma(static_cast<double>(__value)));
  else
    return static_cast<_Float>(__builtin_tgammaf(static_cast<float>(__value)));
}

template <class _Float, __enable_if_t<__is_stdfloat_math_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline _LIBCPP_CONSTEXPR_SINCE_CXX23 _Float lgamma(_Float __value) _NOEXCEPT {
  if constexpr (__is_stdfloat64_v<_Float>)
    return static_cast<_Float>(__builtin_lgamma(static_cast<double>(__value)));
  else
    return static_cast<_Float>(__builtin_lgammaf(static_cast<float>(__value)));
}

template <class _Float, __enable_if_t<__is_stdfloat_math_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline _LIBCPP_CONSTEXPR_SINCE_CXX23 _Float atan2(_Float __left, _Float __right) _NOEXCEPT {
  if constexpr (__is_stdfloat64_v<_Float>)
    return static_cast<_Float>(__builtin_atan2(static_cast<double>(__left), static_cast<double>(__right)));
  else
    return static_cast<_Float>(__builtin_atan2f(static_cast<float>(__left), static_cast<float>(__right)));
}

template <class _Float, __enable_if_t<__is_stdfloat_math_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline _LIBCPP_CONSTEXPR_SINCE_CXX23 _Float copysign(_Float __left, _Float __right) _NOEXCEPT {
  if constexpr (__is_stdfloat64_v<_Float>)
    return static_cast<_Float>(__builtin_copysign(static_cast<double>(__left), static_cast<double>(__right)));
  else
    return static_cast<_Float>(__builtin_copysignf(static_cast<float>(__left), static_cast<float>(__right)));
}

template <class _Float, __enable_if_t<__is_stdfloat_math_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline _LIBCPP_CONSTEXPR_SINCE_CXX23 _Float fdim(_Float __left, _Float __right) _NOEXCEPT {
  if constexpr (__is_stdfloat64_v<_Float>)
    return static_cast<_Float>(__builtin_fdim(static_cast<double>(__left), static_cast<double>(__right)));
  else
    return static_cast<_Float>(__builtin_fdimf(static_cast<float>(__left), static_cast<float>(__right)));
}

template <class _Float, __enable_if_t<__is_stdfloat_math_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline _LIBCPP_CONSTEXPR_SINCE_CXX23 _Float fmax(_Float __left, _Float __right) _NOEXCEPT {
  if constexpr (__is_stdfloat64_v<_Float>)
    return static_cast<_Float>(__builtin_fmax(static_cast<double>(__left), static_cast<double>(__right)));
  else
    return static_cast<_Float>(__builtin_fmaxf(static_cast<float>(__left), static_cast<float>(__right)));
}

template <class _Float, __enable_if_t<__is_stdfloat_math_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline _LIBCPP_CONSTEXPR_SINCE_CXX23 _Float fmin(_Float __left, _Float __right) _NOEXCEPT {
  if constexpr (__is_stdfloat64_v<_Float>)
    return static_cast<_Float>(__builtin_fmin(static_cast<double>(__left), static_cast<double>(__right)));
  else
    return static_cast<_Float>(__builtin_fminf(static_cast<float>(__left), static_cast<float>(__right)));
}

template <class _Float, __enable_if_t<__is_stdfloat_math_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline _LIBCPP_CONSTEXPR_SINCE_CXX23 _Float fmod(_Float __left, _Float __right) _NOEXCEPT {
  if constexpr (__is_stdfloat64_v<_Float>)
    return static_cast<_Float>(__builtin_fmod(static_cast<double>(__left), static_cast<double>(__right)));
  else
    return static_cast<_Float>(__builtin_fmodf(static_cast<float>(__left), static_cast<float>(__right)));
}

template <class _Float, __enable_if_t<__is_stdfloat_math_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline _LIBCPP_CONSTEXPR_SINCE_CXX23 _Float hypot(_Float __left, _Float __right) _NOEXCEPT {
  if constexpr (__is_stdfloat64_v<_Float>)
    return static_cast<_Float>(__builtin_hypot(static_cast<double>(__left), static_cast<double>(__right)));
  else
    return static_cast<_Float>(__builtin_hypotf(static_cast<float>(__left), static_cast<float>(__right)));
}

template <class _Float, __enable_if_t<__is_stdfloat_math_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline _LIBCPP_CONSTEXPR_SINCE_CXX23 _Float nextafter(_Float __left, _Float __right) _NOEXCEPT {
  if constexpr (__is_stdfloat64_v<_Float>)
    return static_cast<_Float>(__builtin_nextafter(static_cast<double>(__left), static_cast<double>(__right)));
  else
    return static_cast<_Float>(__builtin_nextafterf(static_cast<float>(__left), static_cast<float>(__right)));
}

template <class _Float, __enable_if_t<__is_stdfloat_math_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline _LIBCPP_CONSTEXPR_SINCE_CXX23 _Float pow(_Float __left, _Float __right) _NOEXCEPT {
  if constexpr (__is_stdfloat64_v<_Float>)
    return static_cast<_Float>(__builtin_pow(static_cast<double>(__left), static_cast<double>(__right)));
  else
    return static_cast<_Float>(__builtin_powf(static_cast<float>(__left), static_cast<float>(__right)));
}

template <class _Float, __enable_if_t<__is_stdfloat_math_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline _LIBCPP_CONSTEXPR_SINCE_CXX23 _Float remainder(_Float __left, _Float __right) _NOEXCEPT {
  if constexpr (__is_stdfloat64_v<_Float>)
    return static_cast<_Float>(__builtin_remainder(static_cast<double>(__left), static_cast<double>(__right)));
  else
    return static_cast<_Float>(__builtin_remainderf(static_cast<float>(__left), static_cast<float>(__right)));
}

template <class _Float, __enable_if_t<__is_extended_floating_point_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline _LIBCPP_CONSTEXPR_SINCE_CXX23 _Float abs(_Float __value) _NOEXCEPT {
#  if defined(__STDCPP_FLOAT128_T__)
  if constexpr (is_same<_Float, __float128>::value)
    return __builtin_fabsf128(__value);
  else
#  endif
      if constexpr (__is_stdfloat64_v<_Float>)
    return static_cast<_Float>(__builtin_fabs(static_cast<double>(__value)));
  else
    return static_cast<_Float>(__builtin_fabsf(static_cast<float>(__value)));
}

template <class _Float, __enable_if_t<__is_extended_floating_point_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline _LIBCPP_CONSTEXPR_SINCE_CXX23 _Float fabs(_Float __value) _NOEXCEPT {
#  if defined(__STDCPP_FLOAT128_T__)
  if constexpr (is_same<_Float, __float128>::value)
    return __builtin_fabsf128(__value);
  else
#  endif
      if constexpr (__is_stdfloat64_v<_Float>)
    return static_cast<_Float>(__builtin_fabs(static_cast<double>(__value)));
  else
    return static_cast<_Float>(__builtin_fabsf(static_cast<float>(__value)));
}

template <class _Float, __enable_if_t<__is_extended_floating_point_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline _LIBCPP_CONSTEXPR_SINCE_CXX23 _Float sqrt(_Float __value) _NOEXCEPT {
#  if defined(__STDCPP_FLOAT128_T__)
  if constexpr (is_same<_Float, __float128>::value)
    return __builtin_sqrtf128(__value);
  else
#  endif
      if constexpr (__is_stdfloat64_v<_Float>)
    return static_cast<_Float>(__builtin_sqrt(static_cast<double>(__value)));
  else
    return static_cast<_Float>(__builtin_sqrtf(static_cast<float>(__value)));
}

template <class _Float, __enable_if_t<__is_extended_floating_point_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline constexpr bool isnan(_Float __value) _NOEXCEPT {
  return __builtin_isnan(__value);
}

template <class _Float, __enable_if_t<__is_extended_floating_point_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline constexpr bool isinf(_Float __value) _NOEXCEPT {
  return __builtin_isinf(__value);
}

template <class _Float, __enable_if_t<__is_extended_floating_point_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline constexpr bool isfinite(_Float __value) _NOEXCEPT {
  return __builtin_isfinite(__value);
}

template <class _Float, __enable_if_t<__is_extended_floating_point_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline constexpr bool isnormal(_Float __value) _NOEXCEPT {
  return __builtin_isnormal(__value);
}

template <class _Float, __enable_if_t<__is_extended_floating_point_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline constexpr bool signbit(_Float __value) _NOEXCEPT {
  return __builtin_signbit(__value);
}

template <class _Float, __enable_if_t<__is_stdfloat_math_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline _LIBCPP_CONSTEXPR_SINCE_CXX23 int ilogb(_Float __value) _NOEXCEPT {
  if constexpr (__is_stdfloat64_v<_Float>)
    return __builtin_ilogb(static_cast<double>(__value));
  else
    return __builtin_ilogbf(static_cast<float>(__value));
}
template <class _Float, __enable_if_t<__is_stdfloat_math_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline _LIBCPP_CONSTEXPR_SINCE_CXX23 long lrint(_Float __value) _NOEXCEPT {
  if constexpr (__is_stdfloat64_v<_Float>)
    return __builtin_lrint(static_cast<double>(__value));
  else
    return __builtin_lrintf(static_cast<float>(__value));
}
template <class _Float, __enable_if_t<__is_stdfloat_math_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline _LIBCPP_CONSTEXPR_SINCE_CXX23 long lround(_Float __value) _NOEXCEPT {
  if constexpr (__is_stdfloat64_v<_Float>)
    return __builtin_lround(static_cast<double>(__value));
  else
    return __builtin_lroundf(static_cast<float>(__value));
}
template <class _Float, __enable_if_t<__is_stdfloat_math_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline _LIBCPP_CONSTEXPR_SINCE_CXX23 long long llrint(_Float __value) _NOEXCEPT {
  if constexpr (__is_stdfloat64_v<_Float>)
    return __builtin_llrint(static_cast<double>(__value));
  else
    return __builtin_llrintf(static_cast<float>(__value));
}
template <class _Float, __enable_if_t<__is_stdfloat_math_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline _LIBCPP_CONSTEXPR_SINCE_CXX23 long long llround(_Float __value) _NOEXCEPT {
  if constexpr (__is_stdfloat64_v<_Float>)
    return __builtin_llround(static_cast<double>(__value));
  else
    return __builtin_llroundf(static_cast<float>(__value));
}
template <class _Float, __enable_if_t<__is_stdfloat_math_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline _LIBCPP_CONSTEXPR_SINCE_CXX23 _Float ldexp(_Float __value, int __exponent) _NOEXCEPT {
  if constexpr (__is_stdfloat64_v<_Float>)
    return static_cast<_Float>(__builtin_ldexp(static_cast<double>(__value), __exponent));
  else
    return static_cast<_Float>(__builtin_ldexpf(static_cast<float>(__value), __exponent));
}
template <class _Float, __enable_if_t<__is_stdfloat_math_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline _LIBCPP_CONSTEXPR_SINCE_CXX23 _Float scalbn(_Float __value, int __exponent) _NOEXCEPT {
  if constexpr (__is_stdfloat64_v<_Float>)
    return static_cast<_Float>(__builtin_scalbn(static_cast<double>(__value), __exponent));
  else
    return static_cast<_Float>(__builtin_scalbnf(static_cast<float>(__value), __exponent));
}
template <class _Float, __enable_if_t<__is_stdfloat_math_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline _LIBCPP_CONSTEXPR_SINCE_CXX23 _Float scalbln(_Float __value, long __exponent) _NOEXCEPT {
  if constexpr (__is_stdfloat64_v<_Float>)
    return static_cast<_Float>(__builtin_scalbln(static_cast<double>(__value), __exponent));
  else
    return static_cast<_Float>(__builtin_scalblnf(static_cast<float>(__value), __exponent));
}
template <class _Float, __enable_if_t<__is_stdfloat_math_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline _Float frexp(_Float __value, int* __exponent) _NOEXCEPT {
  if constexpr (__is_stdfloat64_v<_Float>)
    return static_cast<_Float>(__builtin_frexp(static_cast<double>(__value), __exponent));
  else
    return static_cast<_Float>(__builtin_frexpf(static_cast<float>(__value), __exponent));
}
template <class _Float, __enable_if_t<__is_stdfloat_math_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline _Float modf(_Float __value, _Float* __integral) _NOEXCEPT {
  if constexpr (__is_stdfloat64_v<_Float>) {
    double __int_part;
    double __result = __builtin_modf(static_cast<double>(__value), &__int_part);
    *__integral     = static_cast<_Float>(__int_part);
    return static_cast<_Float>(__result);
  } else {
    float __int_part;
    float __result = __builtin_modff(static_cast<float>(__value), &__int_part);
    *__integral    = static_cast<_Float>(__int_part);
    return static_cast<_Float>(__result);
  }
}
template <class _Float, __enable_if_t<__is_stdfloat_math_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline _Float remquo(_Float __left, _Float __right, int* __quotient) _NOEXCEPT {
  if constexpr (__is_stdfloat64_v<_Float>)
    return static_cast<_Float>(__builtin_remquo(static_cast<double>(__left), static_cast<double>(__right), __quotient));
  else
    return static_cast<_Float>(__builtin_remquof(static_cast<float>(__left), static_cast<float>(__right), __quotient));
}
template <class _Float, __enable_if_t<__is_stdfloat_math_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline _LIBCPP_CONSTEXPR_SINCE_CXX23 _Float
fma(_Float __left, _Float __right, _Float __addend) _NOEXCEPT {
  if constexpr (__is_stdfloat64_v<_Float>)
    return static_cast<_Float>(
        __builtin_fma(static_cast<double>(__left), static_cast<double>(__right), static_cast<double>(__addend)));
  else
    return static_cast<_Float>(
        __builtin_fmaf(static_cast<float>(__left), static_cast<float>(__right), static_cast<float>(__addend)));
}

template <class _Float, __enable_if_t<__is_stdfloat_math_v<_Float>, int> = 0>
_LIBCPP_HIDE_FROM_ABI inline _Float nexttoward(_Float __left, long double __right) _NOEXCEPT {
  if constexpr (__is_stdfloat64_v<_Float>)
    return static_cast<_Float>(__builtin_nexttoward(static_cast<double>(__left), __right));
  else
    return static_cast<_Float>(__builtin_nexttowardf(static_cast<float>(__left), __right));
}

// glibc exposes the IEEE binary128 libm ABI with an `f128` suffix. Clang has
// no corresponding builtins, so declare and call those ABI entry points.
#  if defined(__STDCPP_FLOAT128_T__) && defined(__GLIBC__) && __GLIBC_PREREQ(2, 26)
extern "C" {
#    define _LIBCPP_F128_UNARY_DECL(__name) __float128 __name##f128(__float128) noexcept;
_LIBCPP_F128_UNARY_DECL(acos)
_LIBCPP_F128_UNARY_DECL(asin)
_LIBCPP_F128_UNARY_DECL(atan)
_LIBCPP_F128_UNARY_DECL(acosh)
_LIBCPP_F128_UNARY_DECL(asinh)
_LIBCPP_F128_UNARY_DECL(atanh)
_LIBCPP_F128_UNARY_DECL(cos)
_LIBCPP_F128_UNARY_DECL(cosh)
_LIBCPP_F128_UNARY_DECL(sin)
_LIBCPP_F128_UNARY_DECL(sinh)
_LIBCPP_F128_UNARY_DECL(tan)
_LIBCPP_F128_UNARY_DECL(tanh)
_LIBCPP_F128_UNARY_DECL(exp)
_LIBCPP_F128_UNARY_DECL(exp2)
_LIBCPP_F128_UNARY_DECL(expm1)
_LIBCPP_F128_UNARY_DECL(log)
_LIBCPP_F128_UNARY_DECL(log2)
_LIBCPP_F128_UNARY_DECL(log10)
_LIBCPP_F128_UNARY_DECL(log1p)
_LIBCPP_F128_UNARY_DECL(cbrt)
_LIBCPP_F128_UNARY_DECL(ceil)
_LIBCPP_F128_UNARY_DECL(floor)
_LIBCPP_F128_UNARY_DECL(round)
_LIBCPP_F128_UNARY_DECL(trunc)
_LIBCPP_F128_UNARY_DECL(nearbyint)
_LIBCPP_F128_UNARY_DECL(rint)
_LIBCPP_F128_UNARY_DECL(erf)
_LIBCPP_F128_UNARY_DECL(erfc)
_LIBCPP_F128_UNARY_DECL(tgamma)
_LIBCPP_F128_UNARY_DECL(lgamma)
_LIBCPP_F128_UNARY_DECL(logb)
_LIBCPP_F128_UNARY_DECL(j0)
_LIBCPP_F128_UNARY_DECL(j1)
_LIBCPP_F128_UNARY_DECL(y0)
_LIBCPP_F128_UNARY_DECL(y1)
#    undef _LIBCPP_F128_UNARY_DECL
__float128 atan2f128(__float128, __float128) noexcept;
__float128 hypotf128(__float128, __float128) noexcept;
__float128 powf128(__float128, __float128) noexcept;
__float128 fmodf128(__float128, __float128) noexcept;
__float128 remainderf128(__float128, __float128) noexcept;
__float128 copysignf128(__float128, __float128) noexcept;
__float128 fdimf128(__float128, __float128) noexcept;
__float128 fmaxf128(__float128, __float128) noexcept;
__float128 fminf128(__float128, __float128) noexcept;
__float128 nextafterf128(__float128, __float128) noexcept;
__float128 fmaf128(__float128, __float128, __float128) noexcept;
__float128 remquof128(__float128, __float128, int*) noexcept;
__float128 frexpf128(__float128, int*) noexcept;
__float128 modff128(__float128, __float128*) noexcept;
__float128 ldexpf128(__float128, int) noexcept;
__float128 scalbnf128(__float128, int) noexcept;
}

#    define _LIBCPP_F128_UNARY(__name) \
      _LIBCPP_HIDE_FROM_ABI inline __float128 __name(__float128 __value) noexcept { return __name##f128(__value); }
_LIBCPP_F128_UNARY(acos)
_LIBCPP_F128_UNARY(asin)
_LIBCPP_F128_UNARY(atan)
_LIBCPP_F128_UNARY(acosh)
_LIBCPP_F128_UNARY(asinh)
_LIBCPP_F128_UNARY(atanh)
_LIBCPP_F128_UNARY(cos)
_LIBCPP_F128_UNARY(cosh)
_LIBCPP_F128_UNARY(sin)
_LIBCPP_F128_UNARY(sinh)
_LIBCPP_F128_UNARY(tan)
_LIBCPP_F128_UNARY(tanh)
_LIBCPP_F128_UNARY(exp)
_LIBCPP_F128_UNARY(exp2)
_LIBCPP_F128_UNARY(expm1)
_LIBCPP_F128_UNARY(log)
_LIBCPP_F128_UNARY(log2)
_LIBCPP_F128_UNARY(log10)
_LIBCPP_F128_UNARY(log1p)
_LIBCPP_F128_UNARY(cbrt)
_LIBCPP_F128_UNARY(ceil)
_LIBCPP_F128_UNARY(floor)
_LIBCPP_F128_UNARY(round)
_LIBCPP_F128_UNARY(trunc)
_LIBCPP_F128_UNARY(nearbyint)
_LIBCPP_F128_UNARY(rint)
_LIBCPP_F128_UNARY(erf)
_LIBCPP_F128_UNARY(erfc)
_LIBCPP_F128_UNARY(tgamma)
_LIBCPP_F128_UNARY(lgamma)
_LIBCPP_F128_UNARY(logb)
#    undef _LIBCPP_F128_UNARY
#    define _LIBCPP_F128_BINARY(__name) \
      _LIBCPP_HIDE_FROM_ABI inline __float128 __name(__float128 __left, __float128 __right) noexcept { return __name##f128(__left, __right); }
_LIBCPP_F128_BINARY(atan2)
_LIBCPP_F128_BINARY(hypot)
_LIBCPP_F128_BINARY(pow)
_LIBCPP_F128_BINARY(fmod)
_LIBCPP_F128_BINARY(remainder)
_LIBCPP_F128_BINARY(copysign)
_LIBCPP_F128_BINARY(fdim)
_LIBCPP_F128_BINARY(fmax)
_LIBCPP_F128_BINARY(fmin)
_LIBCPP_F128_BINARY(nextafter)
#    undef _LIBCPP_F128_BINARY
_LIBCPP_HIDE_FROM_ABI inline __float128 fma(__float128 __x, __float128 __y, __float128 __z) noexcept { return fmaf128(__x, __y, __z); }
_LIBCPP_HIDE_FROM_ABI inline __float128 frexp(__float128 __value, int* __exponent) noexcept { return frexpf128(__value, __exponent); }
_LIBCPP_HIDE_FROM_ABI inline __float128 modf(__float128 __value, __float128* __integer) noexcept { return modff128(__value, __integer); }
_LIBCPP_HIDE_FROM_ABI inline __float128 ldexp(__float128 __value, int __exponent) noexcept { return ldexpf128(__value, __exponent); }
_LIBCPP_HIDE_FROM_ABI inline __float128 scalbn(__float128 __value, int __exponent) noexcept { return scalbnf128(__value, __exponent); }
_LIBCPP_HIDE_FROM_ABI inline __float128 remquo(__float128 __value, __float128 __denominator, int* __quotient) noexcept { return remquof128(__value, __denominator, __quotient); }
#  endif
} // namespace __math
_LIBCPP_END_NAMESPACE_STD
#endif // _LIBCPP_STD_VER >= 23

#endif // _LIBCPP___MATH_STDFLOAT_H
