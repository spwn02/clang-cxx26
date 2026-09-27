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
_LIBCPP_HIDE_FROM_ABI inline to_chars_result to_chars(char* __first, char* __last, float16_t __value) { return std::__to_chars_narrow_float(__first, __last, __value); }
_LIBCPP_HIDE_FROM_ABI inline to_chars_result to_chars(char* __first, char* __last, float16_t __value, chars_format __fmt) { return std::to_chars(__first, __last, static_cast<float>(__value), __fmt); }
_LIBCPP_HIDE_FROM_ABI inline to_chars_result to_chars(char* __first, char* __last, float16_t __value, chars_format __fmt, int __precision) { return std::to_chars(__first, __last, static_cast<float>(__value), __fmt, __precision); }
#    endif
#    if defined(__STDCPP_BFLOAT16_T__)
_LIBCPP_HIDE_FROM_ABI inline to_chars_result to_chars(char* __first, char* __last, bfloat16_t __value) { return std::__to_chars_narrow_float(__first, __last, __value); }
_LIBCPP_HIDE_FROM_ABI inline to_chars_result to_chars(char* __first, char* __last, bfloat16_t __value, chars_format __fmt) { return std::to_chars(__first, __last, static_cast<float>(__value), __fmt); }
_LIBCPP_HIDE_FROM_ABI inline to_chars_result to_chars(char* __first, char* __last, bfloat16_t __value, chars_format __fmt, int __precision) { return std::to_chars(__first, __last, static_cast<float>(__value), __fmt, __precision); }
#    endif
#  endif // _LIBCPP_STD_VER >= 23
#endif // _LIBCPP_STD_VER >= 17

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___CHARCONV_TO_CHARS_FLOATING_POINT_H
