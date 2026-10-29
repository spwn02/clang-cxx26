//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include <__locale>
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

#if (defined(__x86_64__) || defined(__i386__)) && __LDBL_MANT_DIG__ == 64 // the x87 long double (not MSVC, where it is a double)
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
  char* __dst_pos = __head;
  if (__negative)
    *__dst_pos++ = '-';
  *__dst_pos++ = "0123456789abcdef"[__lead];
  const bool __point = __hexits > 0 || __padding > 0;
  if (__point)
    *__dst_pos++ = '.';
  for (int __i = __hexits - 1; __i >= 0; --__i)
    *__dst_pos++ = "0123456789abcdef"[(__frac >> (4 * __i)) & 0xf];

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

  const ptrdiff_t __head_length = __dst_pos - __head;
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

  string __dst_pos;
  if (__negative)
    __dst_pos.push_back('-');
  if (__fmt == chars_format::fixed) {
    if (__abs == 0) {
      __dst_pos.push_back('0');
    } else if (__ryu_exponent >= 0) {
      __dst_pos += __digits;
      __dst_pos.append(static_cast<size_t>(__ryu_exponent), '0');
    } else if (__sci_exponent >= 0) {
      __dst_pos.append(__digits, 0, static_cast<size_t>(__sci_exponent + 1));
      __dst_pos.push_back('.');
      __dst_pos.append(__digits, static_cast<size_t>(__sci_exponent + 1), string::npos);
    } else {
      __dst_pos += "0.";
      __dst_pos.append(static_cast<size_t>(-__sci_exponent - 1), '0');
      __dst_pos += __digits;
    }
  } else { // scientific
    __dst_pos.push_back(__digits[0]);
    if (__length > 1) {
      __dst_pos.push_back('.');
      __dst_pos.append(__digits, 1, string::npos);
    }
    const int __exp = __abs == 0 ? 0 : __sci_exponent;
    __dst_pos.push_back('e');
    __dst_pos.push_back(__exp < 0 ? '-' : '+');
    const int __abs_exp = __exp < 0 ? -__exp : __exp;
    if (__abs_exp < 10)
      __dst_pos.push_back('0');
    __dst_pos += to_string(__abs_exp);
  }

  if (static_cast<ptrdiff_t>(__dst_pos.size()) > __last - __first)
    return {__last, errc::value_too_large};
  ::memcpy(__first, __dst_pos.data(), __dst_pos.size());
  return {__first + __dst_pos.size(), errc{}};
}


to_chars_result __to_chars_long_double(
    char* __first, char* __last, long double __value, chars_format __fmt, int __precision, bool __has_precision) {
#if (defined(__x86_64__) || defined(__i386__)) && __LDBL_MANT_DIG__ == 64 // the x87 long double (not MSVC, where it is a double)
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

namespace {

inline bool __is_digit_in(char __c, bool __hex) {
  return (__c >= '0' && __c <= '9') || (__hex && ((__c >= 'a' && __c <= 'f') || (__c >= 'A' && __c <= 'F')));
}

inline char __lower(char __c) { return (__c >= 'A' && __c <= 'Z') ? static_cast<char>(__c - 'A' + 'a') : __c; }

// Whether [__p, __last) starts with the (case-insensitive) lower case word __w.
inline bool __starts_with_nocase(const char* __p, const char* __last, const char* __w) {
  for (; *__w != '\0'; ++__w, ++__p)
    if (__p == __last || __lower(*__p) != *__w)
      return false;
  return true;
}

} // namespace

// [charconv.from.chars]: the subject sequence is matched here with the grammar of strtold in the "C" locale (no
// leading white space, no '+', no 0x prefix for hex, the exponent as required by the format) and only the matched
// characters are converted, with strtold in the "C" locale.
_LIBCPP_EXPORTED_FROM_ABI __from_chars_result<long double> __from_chars_long_double(
    _LIBCPP_NOESCAPE const char* __first, _LIBCPP_NOESCAPE const char* __last, chars_format __fmt) {
  const char* __p = __first;
  const bool __negative = __p != __last && *__p == '-';
  if (__negative)
    ++__p;

  auto __invalid = [] { return __from_chars_result<long double>{0.0L, 0, errc::invalid_argument}; };
  if (__p == __last)
    return __invalid();

  const long double __sign = __negative ? -1.0L : 1.0L;

  // inf, infinity, nan, nan(n-char-sequence)
  if (__starts_with_nocase(__p, __last, "inf")) {
    __p += 3;
    if (__starts_with_nocase(__p, __last, "inity"))
      __p += 5;
    return {__sign * __builtin_infl(), __p - __first, errc{}};
  }
  if (__starts_with_nocase(__p, __last, "nan")) {
    __p += 3;
    if (__p != __last && *__p == '(') {
      const char* __q = __p + 1;
      while (__q != __last && (__is_digit_in(*__q, false) || (__lower(*__q) >= 'a' && __lower(*__q) <= 'z') || *__q == '_'))
        ++__q;
      if (__q != __last && *__q == ')')
        __p = __q + 1;
    }
    return {__negative ? -__builtin_nanl("") : __builtin_nanl(""), __p - __first, errc{}};
  }

  const bool __hex = __fmt == chars_format::hex;
  const char* __digits_begin = __p;
  bool __nonzero_digit = false;
  size_t __count = 0;
  for (; __p != __last && __is_digit_in(*__p, __hex); ++__p, ++__count)
    __nonzero_digit |= *__p != '0';
  if (__p != __last && *__p == '.') {
    ++__p;
    for (; __p != __last && __is_digit_in(*__p, __hex); ++__p, ++__count)
      __nonzero_digit |= *__p != '0';
  }
  if (__count == 0)
    return __invalid();
  const char* __mantissa_end = __p;

  // The exponent: 'e' for decimal formats, 'p' for hex. It is not part of the fixed format, and it is required
  // by the scientific format.
  bool __has_exponent = false;
  if (__fmt != chars_format::fixed && __p != __last && __lower(*__p) == (__hex ? 'p' : 'e')) {
    const char* __q = __p + 1;
    if (__q != __last && (*__q == '+' || *__q == '-'))
      ++__q;
    const char* __exp_digits = __q;
    while (__q != __last && __is_digit_in(*__q, false))
      ++__q;
    if (__q != __exp_digits) {
      __p            = __q;
      __has_exponent = true;
    }
  }
  if (__fmt == chars_format::scientific && !__has_exponent)
    return __invalid();
  (void)__mantissa_end;

  string __text;
  if (__negative)
    __text += '-';
  if (__hex)
    __text += "0x";
  __text.append(__digits_begin, __p);

  static const __locale::__locale_t __c_locale = __locale::__newlocale(_LIBCPP_ALL_MASK, "C", nullptr);
  char* __end         = nullptr;
  long double __value = __locale::__strtold(__text.c_str(), &__end, __c_locale);

  // Only an overflow, or an underflow to zero of a non-zero number, is out of range; a subnormal result is a
  // representable value.
  if (__builtin_isinf(__value) || (__value == 0.0L && __nonzero_digit))
    return {__value, __p - __first, errc::result_out_of_range};
  return {__value, __p - __first, errc{}};
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
