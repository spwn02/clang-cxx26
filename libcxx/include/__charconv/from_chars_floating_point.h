// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___CHARCONV_FROM_CHARS_FLOATING_POINT_H
#define _LIBCPP___CHARCONV_FROM_CHARS_FLOATING_POINT_H

#include <__assert>
#include <__charconv/chars_format.h>
#include <__charconv/from_chars_result.h>
#include <__config>
#include <__cstddef/ptrdiff_t.h>
#include <__system_error/errc.h>
#if _LIBCPP_STD_VER >= 23
#  include <stdfloat>
#endif

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 17


template <class _Fp>
struct __from_chars_result {
  _Fp __value;
  ptrdiff_t __n;
  errc __ec;
};

template <class _Fp>
_LIBCPP_EXPORTED_FROM_ABI __from_chars_result<_Fp> __from_chars_floating_point(
    _LIBCPP_NOESCAPE const char* __first, _LIBCPP_NOESCAPE const char* __last, chars_format __fmt);

extern template __from_chars_result<float> __from_chars_floating_point(
    _LIBCPP_NOESCAPE const char* __first, _LIBCPP_NOESCAPE const char* __last, chars_format __fmt);

extern template __from_chars_result<double> __from_chars_floating_point(
    _LIBCPP_NOESCAPE const char* __first, _LIBCPP_NOESCAPE const char* __last, chars_format __fmt);

_LIBCPP_EXPORTED_FROM_ABI __from_chars_result<long double> __from_chars_long_double(
    _LIBCPP_NOESCAPE const char* __first, _LIBCPP_NOESCAPE const char* __last, chars_format __fmt);

template <class _Fp>
_LIBCPP_HIDE_FROM_ABI from_chars_result
__from_chars(const char* __first, const char* __last, _Fp& __value, chars_format __fmt) {
  __from_chars_result<_Fp> __r = std::__from_chars_floating_point<_Fp>(__first, __last, __fmt);
  if (__r.__ec != errc::invalid_argument)
    __value = __r.__value;
  return {__first + __r.__n, __r.__ec};
}

_LIBCPP_AVAILABILITY_FROM_CHARS_FLOATING_POINT _LIBCPP_HIDE_FROM_ABI inline from_chars_result
from_chars(const char* __first, const char* __last, float& __value, chars_format __fmt = chars_format::general) {
  return std::__from_chars<float>(__first, __last, __value, __fmt);
}

_LIBCPP_AVAILABILITY_FROM_CHARS_FLOATING_POINT _LIBCPP_HIDE_FROM_ABI inline from_chars_result
from_chars(const char* __first, const char* __last, double& __value, chars_format __fmt = chars_format::general) {
  return std::__from_chars<double>(__first, __last, __value, __fmt);
}

_LIBCPP_AVAILABILITY_FROM_CHARS_FLOATING_POINT _LIBCPP_HIDE_FROM_ABI inline from_chars_result
from_chars(const char* __first, const char* __last, long double& __value, chars_format __fmt = chars_format::general) {
  __from_chars_result<long double> __r = std::__from_chars_long_double(__first, __last, __fmt);
  if (__r.__ec != errc::invalid_argument)
    __value = __r.__value;
  return {__first + __r.__n, __r.__ec};
}

#  if _LIBCPP_STD_VER >= 23
#    if defined(__STDCPP_FLOAT32_T__)
_LIBCPP_HIDE_FROM_ABI inline from_chars_result from_chars(const char* __first, const char* __last, float32_t& __value, chars_format __fmt = chars_format::general) {
  float __temporary;
  from_chars_result __result = std::from_chars(__first, __last, __temporary, __fmt);
  if (__result.ec != errc::invalid_argument) __value = static_cast<float32_t>(__temporary);
  return __result;
}
#    endif
#    if defined(__STDCPP_FLOAT64_T__)
_LIBCPP_HIDE_FROM_ABI inline from_chars_result from_chars(const char* __first, const char* __last, float64_t& __value, chars_format __fmt = chars_format::general) {
  double __temporary;
  from_chars_result __result = std::from_chars(__first, __last, __temporary, __fmt);
  if (__result.ec != errc::invalid_argument) __value = static_cast<float64_t>(__temporary);
  return __result;
}
#    endif
#    if defined(__STDCPP_FLOAT16_T__)
_LIBCPP_HIDE_FROM_ABI inline from_chars_result from_chars(const char* __first, const char* __last, float16_t& __value, chars_format __fmt = chars_format::general) {
  // Parse to float, then perform one correctly-rounded narrowing conversion.
  float __temporary;
  from_chars_result __result = std::from_chars(__first, __last, __temporary, __fmt);
  if (__result.ec != errc::invalid_argument) __value = static_cast<float16_t>(__temporary);
  return __result;
}
#    endif
#    if defined(__STDCPP_BFLOAT16_T__)
_LIBCPP_HIDE_FROM_ABI inline from_chars_result from_chars(const char* __first, const char* __last, bfloat16_t& __value, chars_format __fmt = chars_format::general) {
  // Parse to float, then perform one correctly-rounded narrowing conversion.
  float __temporary;
  from_chars_result __result = std::from_chars(__first, __last, __temporary, __fmt);
  if (__result.ec != errc::invalid_argument) __value = static_cast<bfloat16_t>(__temporary);
  return __result;
}
#    endif
#  endif

#endif // _LIBCPP_STD_VER >= 17

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP___CHARCONV_FROM_CHARS_FLOATING_POINT_H
