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
_LIBCPP_HIDE_FROM_ABI inline to_chars_result to_chars(char* __first, char* __last, float16_t __value) { return std::to_chars(__first, __last, static_cast<float>(__value)); }
_LIBCPP_HIDE_FROM_ABI inline to_chars_result to_chars(char* __first, char* __last, float16_t __value, chars_format __fmt) { return std::to_chars(__first, __last, static_cast<float>(__value), __fmt); }
_LIBCPP_HIDE_FROM_ABI inline to_chars_result to_chars(char* __first, char* __last, float16_t __value, chars_format __fmt, int __precision) { return std::to_chars(__first, __last, static_cast<float>(__value), __fmt, __precision); }
#    endif
#    if defined(__STDCPP_BFLOAT16_T__)
_LIBCPP_HIDE_FROM_ABI inline to_chars_result to_chars(char* __first, char* __last, bfloat16_t __value) { return std::to_chars(__first, __last, static_cast<float>(__value)); }
_LIBCPP_HIDE_FROM_ABI inline to_chars_result to_chars(char* __first, char* __last, bfloat16_t __value, chars_format __fmt) { return std::to_chars(__first, __last, static_cast<float>(__value), __fmt); }
_LIBCPP_HIDE_FROM_ABI inline to_chars_result to_chars(char* __first, char* __last, bfloat16_t __value, chars_format __fmt, int __precision) { return std::to_chars(__first, __last, static_cast<float>(__value), __fmt, __precision); }
#    endif
#  endif // _LIBCPP_STD_VER >= 23
#endif // _LIBCPP_STD_VER >= 17

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___CHARCONV_TO_CHARS_FLOATING_POINT_H
