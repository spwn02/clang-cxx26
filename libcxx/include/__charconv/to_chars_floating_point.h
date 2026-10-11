// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___CHARCONV_TO_CHARS_FLOATING_POINT_H
#define _LIBCPP___CHARCONV_TO_CHARS_FLOATING_POINT_H

#include <__charconv/chars_format.h>
#include <__charconv/float128.h>
#include <__charconv/from_chars_floating_point.h>
#include <__charconv/to_chars_result.h>
#include <__config>
#if _LIBCPP_STD_VER >= 23
#  include <stdfloat>
#endif

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 17


_LIBCPP_AVAILABILITY_TO_CHARS_FLOATING_POINT _LIBCPP_EXPORTED_FROM_ABI to_chars_result
to_chars(char* __first, char* __last, float __value);

_LIBCPP_AVAILABILITY_TO_CHARS_FLOATING_POINT _LIBCPP_EXPORTED_FROM_ABI to_chars_result
to_chars(char* __first, char* __last, double __value);

_LIBCPP_AVAILABILITY_TO_CHARS_FLOATING_POINT _LIBCPP_EXPORTED_FROM_ABI to_chars_result
to_chars(char* __first, char* __last, long double __value);

_LIBCPP_AVAILABILITY_TO_CHARS_FLOATING_POINT _LIBCPP_EXPORTED_FROM_ABI to_chars_result
to_chars(char* __first, char* __last, float __value, chars_format __fmt);

_LIBCPP_AVAILABILITY_TO_CHARS_FLOATING_POINT _LIBCPP_EXPORTED_FROM_ABI to_chars_result
to_chars(char* __first, char* __last, double __value, chars_format __fmt);

_LIBCPP_AVAILABILITY_TO_CHARS_FLOATING_POINT _LIBCPP_EXPORTED_FROM_ABI to_chars_result
to_chars(char* __first, char* __last, long double __value, chars_format __fmt);

_LIBCPP_AVAILABILITY_TO_CHARS_FLOATING_POINT _LIBCPP_EXPORTED_FROM_ABI to_chars_result
to_chars(char* __first, char* __last, float __value, chars_format __fmt, int __precision);

_LIBCPP_AVAILABILITY_TO_CHARS_FLOATING_POINT _LIBCPP_EXPORTED_FROM_ABI to_chars_result
to_chars(char* __first, char* __last, double __value, chars_format __fmt, int __precision);

_LIBCPP_AVAILABILITY_TO_CHARS_FLOATING_POINT _LIBCPP_EXPORTED_FROM_ABI to_chars_result
to_chars(char* __first, char* __last, long double __value, chars_format __fmt, int __precision);

#  if _LIBCPP_STD_VER >= 23
#    if !defined(__SIZEOF_INT128__)
template <class _Fp>
_LIBCPP_HIDE_FROM_ABI to_chars_result __to_chars_narrow_float(char* __first, char* __last, _Fp __value) {
  // Binary16 and bfloat16 need at most five significant decimal digits. Try
  // each precision and retain the first decimal that rounds back to the value.
  // The float conversion is exact on promotion; parsing it and narrowing once
  // checks the target format's rounding interval.
  char __buffer[64];
  for (int __precision = 1; __precision <= 5; ++__precision) {
    to_chars_result __candidate = std::to_chars(__buffer, __buffer + sizeof(__buffer),
                                                static_cast<float>(__value), chars_format::general, __precision);
    if (__candidate.ec != errc{})
      return {__last, __candidate.ec};
    float __parsed = 0;
    from_chars_result __parsed_result = std::from_chars(__buffer, __candidate.ptr, __parsed, chars_format::general);
    if (__parsed_result.ec == errc{} && static_cast<_Fp>(__parsed) == __value) {
      ptrdiff_t __length = __candidate.ptr - __buffer;
      if (__last - __first < __length)
        return {__last, errc::value_too_large};
      for (ptrdiff_t __index = 0; __index != __length; ++__index)
        __first[__index] = __buffer[__index];
      return {__first + __length, errc{}};
    }
  }
  return std::to_chars(__first, __last, static_cast<float>(__value));
}
#    endif

#    if defined(__SIZEOF_INT128__)
_LIBCPP_HIDE_FROM_ABI inline to_chars_result __to_chars_narrow(
    char* __first, char* __last, unsigned short __bits, __f128::__fmt_kind __kind, const __f128::__fp_format& __format) {
  __f128::__to_chars_out __r = __f128::__to_chars_narrow(__first, __last, __bits, __kind, __format);
  return {__r.__ptr, __r.__too_large ? errc::value_too_large : errc{}};
}
#    endif

_LIBCPP_HIDE_FROM_ABI inline __f128::__fmt_kind __to_chars_narrow_kind(chars_format __fmt) {
  switch (__fmt) {
  case chars_format::scientific:
    return __f128::__fmt_kind::__scientific;
  case chars_format::fixed:
    return __f128::__fmt_kind::__fixed;
  case chars_format::hex:
    return __f128::__fmt_kind::__hex;
  default:
    return __f128::__fmt_kind::__general;
  }
}

// P1467R9: the same-format types can use the existing conversion routines.
#    if defined(__STDCPP_FLOAT32_T__)
_LIBCPP_HIDE_FROM_ABI inline to_chars_result to_chars(char* __first, char* __last, float32_t __value) {
  return std::to_chars(__first, __last, static_cast<float>(__value));
}
_LIBCPP_HIDE_FROM_ABI inline to_chars_result to_chars(char* __first, char* __last, float32_t __value, chars_format __fmt) {
  return std::to_chars(__first, __last, static_cast<float>(__value), __fmt);
}
_LIBCPP_HIDE_FROM_ABI inline to_chars_result to_chars(char* __first, char* __last, float32_t __value, chars_format __fmt, int __precision) {
  return std::to_chars(__first, __last, static_cast<float>(__value), __fmt, __precision);
}
#    endif
#    if defined(__STDCPP_FLOAT64_T__)
_LIBCPP_HIDE_FROM_ABI inline to_chars_result to_chars(char* __first, char* __last, float64_t __value) {
  return std::to_chars(__first, __last, static_cast<double>(__value));
}
_LIBCPP_HIDE_FROM_ABI inline to_chars_result to_chars(char* __first, char* __last, float64_t __value, chars_format __fmt) {
  return std::to_chars(__first, __last, static_cast<double>(__value), __fmt);
}
_LIBCPP_HIDE_FROM_ABI inline to_chars_result to_chars(char* __first, char* __last, float64_t __value, chars_format __fmt, int __precision) {
  return std::to_chars(__first, __last, static_cast<double>(__value), __fmt, __precision);
}
#    endif
#    if defined(__STDCPP_FLOAT16_T__)
#      if defined(__SIZEOF_INT128__)
_LIBCPP_HIDE_FROM_ABI inline to_chars_result to_chars(char* __first, char* __last, float16_t __value) { return std::__to_chars_narrow(__first, __last, __builtin_bit_cast(unsigned short, __value), __f128::__fmt_kind::__plain, __f128::__binary16); }
_LIBCPP_HIDE_FROM_ABI inline to_chars_result to_chars(char* __first, char* __last, float16_t __value, chars_format __fmt) { return __fmt == chars_format::hex ? std::to_chars(__first, __last, static_cast<float>(__value), __fmt) : std::__to_chars_narrow(__first, __last, __builtin_bit_cast(unsigned short, __value), std::__to_chars_narrow_kind(__fmt), __f128::__binary16); }
#      else
_LIBCPP_HIDE_FROM_ABI inline to_chars_result to_chars(char* __first, char* __last, float16_t __value) { return std::__to_chars_narrow_float(__first, __last, __value); }
_LIBCPP_HIDE_FROM_ABI inline to_chars_result to_chars(char* __first, char* __last, float16_t __value, chars_format __fmt) { return std::to_chars(__first, __last, static_cast<float>(__value), __fmt); }
#      endif
_LIBCPP_HIDE_FROM_ABI inline to_chars_result to_chars(char* __first, char* __last, float16_t __value, chars_format __fmt, int __precision) { return std::to_chars(__first, __last, static_cast<float>(__value), __fmt, __precision); }
#    endif
#    if defined(__STDCPP_BFLOAT16_T__)
#      if defined(__SIZEOF_INT128__)
_LIBCPP_HIDE_FROM_ABI inline to_chars_result to_chars(char* __first, char* __last, bfloat16_t __value) { return std::__to_chars_narrow(__first, __last, __builtin_bit_cast(unsigned short, __value), __f128::__fmt_kind::__plain, __f128::__bfloat16); }
_LIBCPP_HIDE_FROM_ABI inline to_chars_result to_chars(char* __first, char* __last, bfloat16_t __value, chars_format __fmt) { return __fmt == chars_format::hex ? std::to_chars(__first, __last, static_cast<float>(__value), __fmt) : std::__to_chars_narrow(__first, __last, __builtin_bit_cast(unsigned short, __value), std::__to_chars_narrow_kind(__fmt), __f128::__bfloat16); }
#      else
_LIBCPP_HIDE_FROM_ABI inline to_chars_result to_chars(char* __first, char* __last, bfloat16_t __value) { return std::__to_chars_narrow_float(__first, __last, __value); }
_LIBCPP_HIDE_FROM_ABI inline to_chars_result to_chars(char* __first, char* __last, bfloat16_t __value, chars_format __fmt) { return std::to_chars(__first, __last, static_cast<float>(__value), __fmt); }
#      endif
_LIBCPP_HIDE_FROM_ABI inline to_chars_result to_chars(char* __first, char* __last, bfloat16_t __value, chars_format __fmt, int __precision) { return std::to_chars(__first, __last, static_cast<float>(__value), __fmt, __precision); }
#    endif
#    if defined(__STDCPP_FLOAT128_T__)
// binary128 has no hardware or library support to build on: exact conversions with a small big integer
// (<__charconv/float128.h>) produce the shortest representation and the correctly rounded precision forms.
_LIBCPP_HIDE_FROM_ABI inline to_chars_result __to_chars_float128(
    char* __first, char* __last, float128_t __value, __f128::__fmt_kind __kind, bool __has_precision, int __precision) {
  __f128::__to_chars_out __r = __f128::__to_chars(
      __first, __last, __builtin_bit_cast(__f128::__u128, __value), __kind, __has_precision, __precision);
  return {__r.__ptr, __r.__too_large ? errc::value_too_large : errc{}};
}

_LIBCPP_HIDE_FROM_ABI inline __f128::__fmt_kind __to_chars_float128_kind(chars_format __fmt) {
  switch (__fmt) {
  case chars_format::scientific:
    return __f128::__fmt_kind::__scientific;
  case chars_format::fixed:
    return __f128::__fmt_kind::__fixed;
  case chars_format::hex:
    return __f128::__fmt_kind::__hex;
  default:
    return __f128::__fmt_kind::__general;
  }
}

_LIBCPP_HIDE_FROM_ABI inline to_chars_result to_chars(char* __first, char* __last, float128_t __value) {
  return std::__to_chars_float128(__first, __last, __value, __f128::__fmt_kind::__plain, false, 0);
}
_LIBCPP_HIDE_FROM_ABI inline to_chars_result
to_chars(char* __first, char* __last, float128_t __value, chars_format __fmt) {
  return std::__to_chars_float128(__first, __last, __value, std::__to_chars_float128_kind(__fmt), false, 0);
}
_LIBCPP_HIDE_FROM_ABI inline to_chars_result
to_chars(char* __first, char* __last, float128_t __value, chars_format __fmt, int __precision) {
  return std::__to_chars_float128(__first, __last, __value, std::__to_chars_float128_kind(__fmt), true, __precision);
}
#    endif
#  endif // _LIBCPP_STD_VER >= 23
#endif // _LIBCPP_STD_VER >= 17

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___CHARCONV_TO_CHARS_FLOATING_POINT_H
