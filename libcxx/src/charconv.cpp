//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include <charconv>
#include <errno.h>
#include <limits>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>

#include "include/from_chars_floating_point.h"
#include "include/to_chars_floating_point.h"

_LIBCPP_BEGIN_NAMESPACE_STD

namespace {

to_chars_result __to_chars_long_double(
    char* __first, char* __last, long double __value, chars_format __fmt, int __precision, bool __has_precision) {
  const char* __format;
  int __effective_precision = __precision;
  if (!__has_precision) {
    __effective_precision = numeric_limits<long double>::max_digits10;
    switch (__fmt) {
    case chars_format::hex:
      __format = "%.*La";
      break;
    case chars_format::scientific:
      __format = "%.*Le";
      break;
    case chars_format::fixed:
      __format = "%.*Lf";
      break;
    case chars_format::general:
    default:
      __format = "%.*Lg";
      break;
    }
  } else {
    if (__effective_precision < 0)
      __effective_precision = 6;
    if (__effective_precision >= 1'000'000'000)
      return {__last, errc::value_too_large};
    switch (__fmt) {
    case chars_format::hex:
      __format = "%.*La";
      break;
    case chars_format::scientific:
      __format = "%.*Le";
      break;
    case chars_format::fixed:
      __format = "%.*Lf";
      break;
    case chars_format::general:
    default:
      __format = "%.*Lg";
      break;
    }
  }

  const ptrdiff_t __capacity = __last - __first;
  if (__capacity <= 0)
    return {__last, errc::value_too_large};
  int __result;
  if (__format[4] == 'a')
    __result = ::snprintf(__first, static_cast<size_t>(__capacity), "%.*La", __effective_precision, __value);
  else if (__format[4] == 'e')
    __result = ::snprintf(__first, static_cast<size_t>(__capacity), "%.*Le", __effective_precision, __value);
  else if (__format[4] == 'f')
    __result = ::snprintf(__first, static_cast<size_t>(__capacity), "%.*Lf", __effective_precision, __value);
  else
    __result = ::snprintf(__first, static_cast<size_t>(__capacity), "%.*Lg", __effective_precision, __value);
  if (__result < 0 || static_cast<ptrdiff_t>(__result) >= __capacity)
    return {__last, errc::value_too_large};

  if (__fmt == chars_format::hex) {
    const ptrdiff_t __sign = __first[0] == '-' ? 1 : 0;
    if (__result >= __sign + 2 && __first[__sign] == '0' && __first[__sign + 1] == 'x') {
      ::memmove(__first + __sign, __first + __sign + 2, static_cast<size_t>(__result - __sign - 1));
      __result -= 2;
    }
  }
  return {__first + __result, errc{}};
}

} // namespace

_LIBCPP_EXPORTED_FROM_ABI __from_chars_result<long double> __from_chars_long_double(
    _LIBCPP_NOESCAPE const char* __first, _LIBCPP_NOESCAPE const char* __last, chars_format __fmt) {
  if (__first == __last || *__first == '+')
    return {0.0L, 0, errc::invalid_argument};

  // strtold requires a null-terminated string, while from_chars accepts a
  // bounded character range. Keep the temporary local to this ABI boundary.
  string __input(__first, __last);
  string __parse_input = __input;
  ptrdiff_t __prefix_position = 0;
  ptrdiff_t __inserted = 0;
  if (__fmt == chars_format::hex && __input.find("0x") == string::npos &&
      __input.find("0X") == string::npos) {
    __prefix_position = __input[0] == '-' ? 1 : 0;
    __parse_input.insert(static_cast<size_t>(__prefix_position), "0x");
    __inserted = 2;
  }
  char* __end = nullptr;
  errno = 0;
  long double __value = ::strtold(__parse_input.c_str(), &__end);
  if (__end == __parse_input.c_str())
    return {0.0L, 0, errc::invalid_argument};

  const ptrdiff_t __n = __end - __parse_input.c_str() - __inserted;
  if (__fmt == chars_format::scientific &&
      __input.find_first_of("eE", 0) == string::npos)
    return {0.0L, 0, errc::invalid_argument};
  if (errno == ERANGE)
    return {__value, __n, errc::result_out_of_range};
  return {__value, __n, errc{}};
}

#if _LIBCPP_AVAILABILITY_MINIMUM_HEADER_VERSION < 15

namespace __itoa {

_LIBCPP_DIAGNOSTIC_PUSH
_LIBCPP_CLANG_DIAGNOSTIC_IGNORED("-Wmissing-prototypes")
// These functions exist for ABI compatibility, so we don't ever want a declaration prior to the definition.
_LIBCPP_EXPORTED_FROM_ABI char* __u32toa(uint32_t value, char* buffer) noexcept { return __base_10_u32(buffer, value); }
_LIBCPP_EXPORTED_FROM_ABI char* __u64toa(uint64_t value, char* buffer) noexcept { return __base_10_u64(buffer, value); }
_LIBCPP_DIAGNOSTIC_POP

} // namespace __itoa

#endif // _LIBCPP_AVAILABILITY_MINIMUM_HEADER_VERSION < 15

// The original version of floating-point to_chars was written by Microsoft and
// contributed with the following license.

// Copyright (c) Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// This implementation is dedicated to the memory of Mary and Thavatchai.

to_chars_result to_chars(char* __first, char* __last, float __value) {
  return _Floating_to_chars<_Floating_to_chars_overload::_Plain>(__first, __last, __value, chars_format{}, 0);
}

to_chars_result to_chars(char* __first, char* __last, double __value) {
  return _Floating_to_chars<_Floating_to_chars_overload::_Plain>(__first, __last, __value, chars_format{}, 0);
}

to_chars_result to_chars(char* __first, char* __last, long double __value) {
  return __to_chars_long_double(__first, __last, __value, chars_format::general, 0, false);
}

to_chars_result to_chars(char* __first, char* __last, float __value, chars_format __fmt) {
  return _Floating_to_chars<_Floating_to_chars_overload::_Format_only>(__first, __last, __value, __fmt, 0);
}

to_chars_result to_chars(char* __first, char* __last, double __value, chars_format __fmt) {
  return _Floating_to_chars<_Floating_to_chars_overload::_Format_only>(__first, __last, __value, __fmt, 0);
}

to_chars_result to_chars(char* __first, char* __last, long double __value, chars_format __fmt) {
  return __to_chars_long_double(__first, __last, __value, __fmt, 0, false);
}

to_chars_result to_chars(char* __first, char* __last, float __value, chars_format __fmt, int __precision) {
  return _Floating_to_chars<_Floating_to_chars_overload::_Format_precision>(
      __first, __last, __value, __fmt, __precision);
}

to_chars_result to_chars(char* __first, char* __last, double __value, chars_format __fmt, int __precision) {
  return _Floating_to_chars<_Floating_to_chars_overload::_Format_precision>(
      __first, __last, __value, __fmt, __precision);
}

to_chars_result to_chars(char* __first, char* __last, long double __value, chars_format __fmt, int __precision) {
  return __to_chars_long_double(__first, __last, __value, __fmt, __precision, true);
}

template <class _Fp>
__from_chars_result<_Fp> __from_chars_floating_point(
    _LIBCPP_NOESCAPE const char* __first, _LIBCPP_NOESCAPE const char* __last, chars_format __fmt) {
  return std::__from_chars_floating_point_impl<_Fp>(__first, __last, __fmt);
}

template __from_chars_result<float> __from_chars_floating_point(
    _LIBCPP_NOESCAPE const char* __first, _LIBCPP_NOESCAPE const char* __last, chars_format __fmt);

template __from_chars_result<double> __from_chars_floating_point(
    _LIBCPP_NOESCAPE const char* __first, _LIBCPP_NOESCAPE const char* __last, chars_format __fmt);

_LIBCPP_END_NAMESPACE_STD
