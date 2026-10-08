//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___MATH_MIN_MAX_H
#define _LIBCPP___MATH_MIN_MAX_H

#include <__config>
#include <__type_traits/enable_if.h>
#include <__type_traits/is_arithmetic.h>
#include <__type_traits/is_same.h>
#include <__type_traits/promote.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

namespace __math {

// fmax

[[__nodiscard__]] inline _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX23 float fmax(float __x,
                                                                                        float __y) _NOEXCEPT {
  return __builtin_fmaxf(__x, __y);
}

template <class = int>
[[__nodiscard__]] _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX23 double fmax(double __x, double __y) _NOEXCEPT {
  return __builtin_fmax(__x, __y);
}

[[__nodiscard__]] inline _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX23 long double
fmax(long double __x, long double __y) _NOEXCEPT {
  return __builtin_fmaxl(__x, __y);
}

template <class _A1, class _A2, __enable_if_t<is_arithmetic<_A1>::value && is_arithmetic<_A2>::value, int> = 0>
[[__nodiscard__]] inline _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX23 __promote_t<_A1, _A2>
fmax(_A1 __x, _A2 __y) _NOEXCEPT {
  using __result_type = __promote_t<_A1, _A2>;
  static_assert(!(_IsSame<_A1, __result_type>::value && _IsSame<_A2, __result_type>::value), "");
  return __math::fmax((__result_type)__x, (__result_type)__y);
}

// fmin

[[__nodiscard__]] inline _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX23 float fmin(float __x,
                                                                                        float __y) _NOEXCEPT {
  return __builtin_fminf(__x, __y);
}

template <class = int>
[[__nodiscard__]] _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX23 double fmin(double __x, double __y) _NOEXCEPT {
  return __builtin_fmin(__x, __y);
}

[[__nodiscard__]] inline _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX23 long double
fmin(long double __x, long double __y) _NOEXCEPT {
  return __builtin_fminl(__x, __y);
}

template <class _A1, class _A2, __enable_if_t<is_arithmetic<_A1>::value && is_arithmetic<_A2>::value, int> = 0>
[[__nodiscard__]] inline _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX23 __promote_t<_A1, _A2>
fmin(_A1 __x, _A2 __y) _NOEXCEPT {
  using __result_type = __promote_t<_A1, _A2>;
  static_assert(!(_IsSame<_A1, __result_type>::value && _IsSame<_A2, __result_type>::value), "");
  return __math::fmin((__result_type)__x, (__result_type)__y);
}

#if _LIBCPP_STD_VER >= 26
// The ISO C 7.12.12 fmaximum/fminimum family: the "_num" variants return the number if exactly one argument is a NaN,
// the others return a NaN; all of them order -0 before +0.

template <class _Tp>
[[__nodiscard__]] _LIBCPP_HIDE_FROM_ABI constexpr _Tp __fmaximum_impl(_Tp __x, _Tp __y, bool __ignore_nan) _NOEXCEPT {
  if (__builtin_isnan(__x))
    return __ignore_nan && !__builtin_isnan(__y) ? __y : __x;
  if (__builtin_isnan(__y))
    return __ignore_nan ? __x : __y;
  if (__x == __y)
    return __builtin_signbit(__x) ? __y : __x;
  return __x > __y ? __x : __y;
}

template <class _Tp>
[[__nodiscard__]] _LIBCPP_HIDE_FROM_ABI constexpr _Tp __fminimum_impl(_Tp __x, _Tp __y, bool __ignore_nan) _NOEXCEPT {
  if (__builtin_isnan(__x))
    return __ignore_nan && !__builtin_isnan(__y) ? __y : __x;
  if (__builtin_isnan(__y))
    return __ignore_nan ? __x : __y;
  if (__x == __y)
    return __builtin_signbit(__x) ? __x : __y;
  return __x < __y ? __x : __y;
}

// fmaximum

[[__nodiscard__]] inline _LIBCPP_HIDE_FROM_ABI constexpr float fmaximum(float __x, float __y) _NOEXCEPT {
  return __math::__fmaximum_impl(__x, __y, false);
}

template <class = int>
[[__nodiscard__]] _LIBCPP_HIDE_FROM_ABI constexpr double fmaximum(double __x, double __y) _NOEXCEPT {
  return __math::__fmaximum_impl(__x, __y, false);
}

[[__nodiscard__]] inline _LIBCPP_HIDE_FROM_ABI constexpr long double fmaximum(long double __x, long double __y) _NOEXCEPT {
  return __math::__fmaximum_impl(__x, __y, false);
}

template <class _A1, class _A2, __enable_if_t<is_arithmetic<_A1>::value && is_arithmetic<_A2>::value, int> = 0>
[[__nodiscard__]] inline _LIBCPP_HIDE_FROM_ABI constexpr __promote_t<_A1, _A2> fmaximum(_A1 __x, _A2 __y) _NOEXCEPT {
  using __result_type = __promote_t<_A1, _A2>;
  static_assert(!(_IsSame<_A1, __result_type>::value && _IsSame<_A2, __result_type>::value), "");
  return __math::fmaximum((__result_type)__x, (__result_type)__y);
}

// fmaximum_num

[[__nodiscard__]] inline _LIBCPP_HIDE_FROM_ABI constexpr float fmaximum_num(float __x, float __y) _NOEXCEPT {
  return __math::__fmaximum_impl(__x, __y, true);
}

template <class = int>
[[__nodiscard__]] _LIBCPP_HIDE_FROM_ABI constexpr double fmaximum_num(double __x, double __y) _NOEXCEPT {
  return __math::__fmaximum_impl(__x, __y, true);
}

[[__nodiscard__]] inline _LIBCPP_HIDE_FROM_ABI constexpr long double fmaximum_num(long double __x, long double __y) _NOEXCEPT {
  return __math::__fmaximum_impl(__x, __y, true);
}

template <class _A1, class _A2, __enable_if_t<is_arithmetic<_A1>::value && is_arithmetic<_A2>::value, int> = 0>
[[__nodiscard__]] inline _LIBCPP_HIDE_FROM_ABI constexpr __promote_t<_A1, _A2> fmaximum_num(_A1 __x, _A2 __y) _NOEXCEPT {
  using __result_type = __promote_t<_A1, _A2>;
  static_assert(!(_IsSame<_A1, __result_type>::value && _IsSame<_A2, __result_type>::value), "");
  return __math::fmaximum_num((__result_type)__x, (__result_type)__y);
}

// fminimum

[[__nodiscard__]] inline _LIBCPP_HIDE_FROM_ABI constexpr float fminimum(float __x, float __y) _NOEXCEPT {
  return __math::__fminimum_impl(__x, __y, false);
}

template <class = int>
[[__nodiscard__]] _LIBCPP_HIDE_FROM_ABI constexpr double fminimum(double __x, double __y) _NOEXCEPT {
  return __math::__fminimum_impl(__x, __y, false);
}

[[__nodiscard__]] inline _LIBCPP_HIDE_FROM_ABI constexpr long double fminimum(long double __x, long double __y) _NOEXCEPT {
  return __math::__fminimum_impl(__x, __y, false);
}

template <class _A1, class _A2, __enable_if_t<is_arithmetic<_A1>::value && is_arithmetic<_A2>::value, int> = 0>
[[__nodiscard__]] inline _LIBCPP_HIDE_FROM_ABI constexpr __promote_t<_A1, _A2> fminimum(_A1 __x, _A2 __y) _NOEXCEPT {
  using __result_type = __promote_t<_A1, _A2>;
  static_assert(!(_IsSame<_A1, __result_type>::value && _IsSame<_A2, __result_type>::value), "");
  return __math::fminimum((__result_type)__x, (__result_type)__y);
}

// fminimum_num

[[__nodiscard__]] inline _LIBCPP_HIDE_FROM_ABI constexpr float fminimum_num(float __x, float __y) _NOEXCEPT {
  return __math::__fminimum_impl(__x, __y, true);
}

template <class = int>
[[__nodiscard__]] _LIBCPP_HIDE_FROM_ABI constexpr double fminimum_num(double __x, double __y) _NOEXCEPT {
  return __math::__fminimum_impl(__x, __y, true);
}

[[__nodiscard__]] inline _LIBCPP_HIDE_FROM_ABI constexpr long double fminimum_num(long double __x, long double __y) _NOEXCEPT {
  return __math::__fminimum_impl(__x, __y, true);
}

template <class _A1, class _A2, __enable_if_t<is_arithmetic<_A1>::value && is_arithmetic<_A2>::value, int> = 0>
[[__nodiscard__]] inline _LIBCPP_HIDE_FROM_ABI constexpr __promote_t<_A1, _A2> fminimum_num(_A1 __x, _A2 __y) _NOEXCEPT {
  using __result_type = __promote_t<_A1, _A2>;
  static_assert(!(_IsSame<_A1, __result_type>::value && _IsSame<_A2, __result_type>::value), "");
  return __math::fminimum_num((__result_type)__x, (__result_type)__y);
}
#endif // _LIBCPP_STD_VER >= 26

} // namespace __math

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___MATH_MIN_MAX_H
