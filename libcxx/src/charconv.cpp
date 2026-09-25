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

#if defined(__x86_64__) || defined(__i386__)
// glibc prints the x87 80-bit long double with "%La" using the explicit integer bit as the leading
// hexit ("0xd.5ep-3" for 0x1.abcp+0), but [charconv.to.chars] requires the normalized form with a single
// leading "1" (or "0" for zero and subnormals), exactly like the float and double implementations.
// So format the hexits ourselves. Only used for finite values when long double is the x87 type.
to_chars_result __to_chars_x87_hex(char* __first, char* __last, long double __value, int __precision, bool __has_precision) {
  static_assert(numeric_limits<long double>::digits == 64, "x87 extended precision layout required");
  unsigned long long __mantissa;
  unsigned short __sign_exponent;
  ::memcpy(&__mantissa, &__value, 8);
  ::memcpy(&__sign_exponent, reinterpret_cast<const char*>(&__value) + 8, 2);
  const bool __negative     = (__sign_exponent & 0x8000) != 0;
  const int __biased        = __sign_exponent & 0x7fff;
  const unsigned __leading  = static_cast<unsigned>(__mantissa >> 63); // the explicit integer bit
  const unsigned long long __fraction = __mantissa << 1;               // 63 fraction bits, aligned to 16 hexits

  int __exponent;
  if (__mantissa == 0)
    __exponent = 0; // C11 7.21.6.1/8: "If the value is zero, the exponent is zero."
  else if (__biased == 0)
    __exponent = 1 - 16383; // subnormal
  else
    __exponent = __biased - 16383;

  // Hexits of the fraction and the leading hexit, after rounding if a precision was requested.
  unsigned __lead = __leading;
  unsigned long long __frac = __fraction;
  int __hexits;
  if (!__has_precision) {
    __hexits = 16;
    while (__hexits > 0 && (__frac & 0xf) == 0) {
      __frac >>= 4;
      --__hexits;
    }
  } else if (__precision >= 16) {
    __hexits = 16;
  } else {
    __hexits = __precision;
    const int __dropped = (16 - __precision) * 4; // 4..64
    unsigned __int128 __combined = (static_cast<unsigned __int128>(__leading) << 64) | __fraction;
    unsigned __int128 __quotient = __combined >> __dropped;
    const unsigned __int128 __remainder = __combined & ((static_cast<unsigned __int128>(1) << __dropped) - 1);
    const unsigned __int128 __half      = static_cast<unsigned __int128>(1) << (__dropped - 1);
    if (__remainder > __half || (__remainder == __half && (__quotient & 1)))
      ++__quotient;
    __lead = static_cast<unsigned>(__quotient >> (4 * __precision));
    __frac = __precision == 0 ? 0ull : static_cast<unsigned long long>(__quotient & ((static_cast<unsigned __int128>(1) << (4 * __precision)) - 1));
  }

  const int __padding = __has_precision && __precision > 16 ? __precision - 16 : 0;

  char __head[32];
  char* __out = __head;
  if (__negative)
    *__out++ = '-';
  *__out++ = "0123456789abcdef"[__lead];
  const bool __point = __hexits > 0 || __padding > 0;
  if (__point)
    *__out++ = '.';
  for (int __i = __hexits - 1; __i >= 0; --__i)
    *__out++ = "0123456789abcdef"[(__frac >> (4 * __i)) & 0xf];

  char __tail[16];
  char* __tail_out = __tail;
  *__tail_out++ = 'p';
  unsigned __abs_exponent = __exponent < 0 ? static_cast<unsigned>(-__exponent) : static_cast<unsigned>(__exponent);
  *__tail_out++ = __exponent < 0 ? '-' : '+';
  char __digits[8];
  int __ndigits = 0;
  do {
    __digits[__ndigits++] = static_cast<char>('0' + __abs_exponent % 10);
    __abs_exponent /= 10;
  } while (__abs_exponent != 0);
  while (__ndigits > 0)
    *__tail_out++ = __digits[--__ndigits];

  const ptrdiff_t __head_length = __out - __head;
  const ptrdiff_t __tail_length = __tail_out - __tail;
  const ptrdiff_t __capacity    = __last - __first;
  if (static_cast<ptrdiff_t>(__padding) + __head_length + __tail_length > __capacity)
    return {__last, errc::value_too_large};
  ::memcpy(__first, __head, static_cast<size_t>(__head_length));
  __first += __head_length;
  ::memset(__first, '0', static_cast<size_t>(__padding));
  __first += __padding;
  ::memcpy(__first, __tail, static_cast<size_t>(__tail_length));
  return {__first + __tail_length, errc{}};
}
#endif // x86


// Shortest round-trip formatting for long double. There is no Ryu implementation for the extended
// types, so search for the smallest number of significant digits whose correctly rounded decimal
// form (glibc's printf is exact) reads back as the same value, then lay the digits out following the
// same rules the float/double implementation uses (see ryu/d2s.cpp).
to_chars_result __to_chars_long_double_shortest(char* __first, char* __last, long double __value, chars_format __fmt) {
  char __buffer[96];
  int __negative = __builtin_signbit(__value) ? 1 : 0;
  const long double __abs = __negative ? -__value : __value;

  string __digits;
  int __sci_exponent = 0;
  if (__abs == 0) {
    __digits = "0";
  } else {
    for (int __digits_count = 1; __digits_count <= numeric_limits<long double>::max_digits10; ++__digits_count) {
      int __n = ::snprintf(__buffer, sizeof(__buffer), "%.*Le", __digits_count - 1, __abs);
      if (__n <= 0 || static_cast<size_t>(__n) >= sizeof(__buffer))
        return {__last, errc::value_too_large};
      char* __end;
      const long double __back = ::strtold(__buffer, &__end);
      if (__back == __abs || __digits_count == numeric_limits<long double>::max_digits10) {
        __digits.clear();
        const char* __e = ::strchr(__buffer, 'e');
        for (const char* __c = __buffer; __c != __e; ++__c)
          if (*__c != '.')
            __digits.push_back(*__c);
        __sci_exponent = static_cast<int>(::strtol(__e + 1, nullptr, 10));
        break;
      }
    }
  }

  const int __length            = static_cast<int>(__digits.size());
  const int __ryu_exponent      = __sci_exponent - (__length - 1);
  if (__fmt == chars_format{}) {
    int __lower, __upper;
    if (__length == 1) {
      __lower = -3;
      __upper = 4;
    } else {
      __lower = -(__length + 3);
      __upper = 5;
    }
    __fmt = (__lower <= __ryu_exponent && __ryu_exponent <= __upper) ? chars_format::fixed : chars_format::scientific;
  } else if (__fmt == chars_format::general) {
    __fmt = (-4 <= __sci_exponent && __sci_exponent < 6) ? chars_format::fixed : chars_format::scientific;
  }

  string __out;
  if (__negative)
    __out.push_back('-');
  if (__fmt == chars_format::fixed) {
    if (__abs == 0) {
      __out.push_back('0');
    } else if (__ryu_exponent >= 0) {
      __out += __digits;
      __out.append(static_cast<size_t>(__ryu_exponent), '0');
    } else if (__sci_exponent >= 0) {
      __out.append(__digits, 0, static_cast<size_t>(__sci_exponent + 1));
      __out.push_back('.');
      __out.append(__digits, static_cast<size_t>(__sci_exponent + 1), string::npos);
    } else {
      __out += "0.";
      __out.append(static_cast<size_t>(-__sci_exponent - 1), '0');
      __out += __digits;
    }
  } else { // scientific
    __out.push_back(__digits[0]);
    if (__length > 1) {
      __out.push_back('.');
      __out.append(__digits, 1, string::npos);
    }
    const int __exp = __abs == 0 ? 0 : __sci_exponent;
    __out.push_back('e');
    __out.push_back(__exp < 0 ? '-' : '+');
    const int __abs_exp = __exp < 0 ? -__exp : __exp;
    if (__abs_exp < 10)
      __out.push_back('0');
    __out += to_string(__abs_exp);
  }

  if (static_cast<ptrdiff_t>(__out.size()) > __last - __first)
    return {__last, errc::value_too_large};
  ::memcpy(__first, __out.data(), __out.size());
  return {__first + __out.size(), errc{}};
}


to_chars_result __to_chars_long_double(
    char* __first, char* __last, long double __value, chars_format __fmt, int __precision, bool __has_precision) {
#if defined(__x86_64__) || defined(__i386__)
  if (__fmt == chars_format::hex && numeric_limits<long double>::digits == 64 && __builtin_isfinite(__value)) {
    if (__has_precision && __precision >= 1'000'000'000)
      return {__last, errc::value_too_large};
    return __to_chars_x87_hex(__first, __last, __value, __has_precision && __precision < 0 ? 6 : __precision, __has_precision);
  }
#endif
  if (!__has_precision && __fmt != chars_format::hex && __builtin_isfinite(__value))
    return __to_chars_long_double_shortest(__first, __last, __value, __fmt);
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
  return __to_chars_long_double(__first, __last, __value, chars_format{}, 0, false);
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
