// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___CHRONO_FROM_STREAM_H
#define _LIBCPP___CHRONO_FROM_STREAM_H

#include <__config>

#if _LIBCPP_HAS_LOCALIZATION

#  include <__chrono/calendar.h>
#  include <__chrono/day.h>
#  include <__chrono/duration.h>
#  include <__chrono/file_clock.h>
#  include <__chrono/hh_mm_ss.h>
#  include <__chrono/month.h>
#  include <__chrono/monthday.h>
#  include <__chrono/system_clock.h>
#  include <__chrono/time_point.h>
#  include <__chrono/weekday.h>
#  include <__chrono/year.h>
#  include <__chrono/year_month.h>
#  include <__chrono/year_month_day.h>
#  include <__cstddef/size_t.h>
#  include <__fwd/istream.h>
#  include <__locale>
#  include <__type_traits/is_floating_point.h>
#  include <__utility/declval.h>
#  include <climits>
#  include <cmath>
#  include <ctime>
#  include <istream>
#  include <limits>
#  include <locale>
#  include <string>

#  if _LIBCPP_HAS_TIME_ZONE_DATABASE && _LIBCPP_HAS_FILESYSTEM && _LIBCPP_HAS_EXPERIMENTAL_TZDB
#    include <__chrono/gps_clock.h>
#    include <__chrono/tai_clock.h>
#    include <__chrono/utc_clock.h>
#  endif

#  if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#    pragma GCC system_header
#  endif

_LIBCPP_PUSH_MACROS
#  include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

#  if _LIBCPP_STD_VER >= 20

namespace chrono {

// [time.parse], parsing.
//
// The implementation parses the format string and collects every field it finds into a __parsed_fields object.
// A per-type conversion then decides whether the collected fields describe a complete value of the destination
// type (a day, a year_month_day, a sys_time, a duration, ...).

namespace __from_stream_detail {

struct __parsed_fields {
  // Calendar fields. -1 means "not parsed" for the fields whose valid range is non-negative; the year, century and
  // ISO year can be negative so they carry an explicit flag.
  int __year_        = 0; // %Y
  int __century_     = 0; // %C
  int __yy_          = -1; // %y
  int __iso_year_    = 0; // %G
  int __iso_yy_      = -1; // %g
  int __month_       = -1;
  int __day_         = -1;
  int __yday_        = -1; // %j (day of year) or the number of days for a duration
  int __week_sun_    = -1; // %U
  int __week_mon_    = -1; // %W
  int __iso_week_    = -1; // %V
  int __weekday_     = -1; // 0..6, Sunday == 0
  bool __has_year_    = false;
  bool __has_century_ = false;
  bool __has_iso_year_ = false;

  // Time of day fields.
  int __hour24_      = -1; // %H
  int __hour12_      = -1; // %I
  int __ampm_        = -1; // 0 == AM, 1 == PM
  int __minute_      = -1;
  long double __second_ = -1; // %S, may carry a fractional part
  bool __has_second_ = false;

  bool __negative_  = false; // leading '-' when parsing a duration
  bool __has_offset_ = false;
  int __offset_minutes_ = 0;

  _LIBCPP_HIDE_FROM_ABI bool __has_date_fields() const {
    return __has_year_ || __has_century_ || __yy_ != -1 || __has_iso_year_ || __iso_yy_ != -1 || __month_ != -1 ||
           __day_ != -1 || __week_sun_ != -1 || __week_mon_ != -1 || __iso_week_ != -1 || __weekday_ != -1;
  }
  _LIBCPP_HIDE_FROM_ABI bool __has_time_fields() const {
    return __hour24_ != -1 || __hour12_ != -1 || __minute_ != -1 || __has_second_;
  }
};

// Stream access helpers. All of them operate directly on the stream buffer because from_stream behaves as an
// unformatted input function.
template <class _CharT, class _Traits>
class __reader {
public:
  using __int_type = typename _Traits::int_type;

  _LIBCPP_HIDE_FROM_ABI __reader(basic_istream<_CharT, _Traits>& __is)
      : __is_(__is),
        __sb_(__is.rdbuf()),
        __ct_(use_facet<ctype<_CharT>>(__is.getloc())),
        __np_(use_facet<numpunct<_CharT>>(__is.getloc())) {}

  _LIBCPP_HIDE_FROM_ABI bool __at_eof() {
    if (_Traits::eq_int_type(__sb_->sgetc(), _Traits::eof())) {
      __state_ |= ios_base::eofbit;
      return true;
    }
    return false;
  }
  _LIBCPP_HIDE_FROM_ABI _CharT __peek() { return _Traits::to_char_type(__sb_->sgetc()); }
  _LIBCPP_HIDE_FROM_ABI void __bump() { __sb_->sbumpc(); }

  _LIBCPP_HIDE_FROM_ABI char __narrow(_CharT __c) const { return __ct_.narrow(__c, '\0'); }
  _LIBCPP_HIDE_FROM_ABI _CharT __widen(char __c) const { return __ct_.widen(__c); }

  _LIBCPP_HIDE_FROM_ABI bool __is_digit(_CharT __c) const {
    char __n = __narrow(__c);
    return __n >= '0' && __n <= '9';
  }
  _LIBCPP_HIDE_FROM_ABI bool __is_space(_CharT __c) const { return __ct_.is(ctype_base::space, __c); }

  // Matches an ordinary character of the format string.
  _LIBCPP_HIDE_FROM_ABI bool __expect(_CharT __c) {
    if (__at_eof())
      return false;
    if (!_Traits::eq(__peek(), __c))
      return false;
    __bump();
    return true;
  }
  _LIBCPP_HIDE_FROM_ABI bool __expect_narrow(char __c) { return __expect(__widen(__c)); }

  _LIBCPP_HIDE_FROM_ABI void __skip_whitespace() {
    while (!__at_eof() && __is_space(__peek()))
      __bump();
  }

  // Reads at most __max_digits decimal digits (at least one). An optional sign is accepted when __allow_sign is set.
  _LIBCPP_HIDE_FROM_ABI bool __read_int(int& __result, int __max_digits, bool __allow_sign = false) {
    bool __negative = false;
    if (__allow_sign && !__at_eof()) {
      char __n = __narrow(__peek());
      if (__n == '-' || __n == '+') {
        __negative = __n == '-';
        __bump();
      }
    }
    long long __value = 0;
    int __digits      = 0;
    while (__digits < __max_digits && !__at_eof() && __is_digit(__peek())) {
      __value = __value * 10 + (__narrow(__peek()) - '0');
      __bump();
      ++__digits;
    }
    if (__digits == 0)
      return false;
    if (__value > INT_MAX)
      return false;
    __result = static_cast<int>(__negative ? -__value : __value);
    return true;
  }

  // Reads seconds with an optional fractional part, at most __max_chars characters in total.
  _LIBCPP_HIDE_FROM_ABI bool __read_seconds(long double& __result, int __max_chars) {
    long double __value = 0;
    int __consumed      = 0;
    bool __any          = false;
    while (__consumed < __max_chars && !__at_eof() && __is_digit(__peek())) {
      __value = __value * 10 + (__narrow(__peek()) - '0');
      __bump();
      ++__consumed;
      __any = true;
    }
    if (!__any)
      return false;
    if (__consumed < __max_chars && !__at_eof() && _Traits::eq(__peek(), __np_.decimal_point())) {
      __bump();
      ++__consumed;
      long double __scale = 1;
      while (__consumed < __max_chars && !__at_eof() && __is_digit(__peek())) {
        __scale /= 10;
        __value += (__narrow(__peek()) - '0') * __scale;
        __bump();
        ++__consumed;
      }
    }
    __result = __value;
    return true;
  }

  // Reads a "single word" for %Z.
  _LIBCPP_HIDE_FROM_ABI bool __read_zone_word(basic_string<_CharT, _Traits>& __result) {
    basic_string<_CharT, _Traits> __word;
    while (!__at_eof()) {
      _CharT __c = __peek();
      char __n   = __narrow(__c);
      bool __ok  = (__n >= 'a' && __n <= 'z') || (__n >= 'A' && __n <= 'Z') || (__n >= '0' && __n <= '9') ||
                  __n == '_' || __n == '/' || __n == '-' || __n == '+';
      if (!__ok)
        break;
      __word.push_back(__c);
      __bump();
    }
    if (__word.empty())
      return false;
    __result = std::move(__word);
    return true;
  }

  // Delegates to the locale's time_get facet for the specifiers that depend on the locale.
  _LIBCPP_HIDE_FROM_ABI bool __time_get(char __spec, tm& __t) {
    const time_get<_CharT>& __tg = use_facet<time_get<_CharT>>(__is_.getloc());
    istreambuf_iterator<_CharT, _Traits> __b(__sb_), __e;
    ios_base::iostate __err = ios_base::goodbit;
    _CharT __fmt[2]         = {__widen('%'), __widen(__spec)};
    __b                     = __tg.get(__b, __e, __is_, __err, &__t, __fmt, __fmt + 2);
    if (__err & ios_base::eofbit)
      __state_ |= ios_base::eofbit;
    return !(__err & ios_base::failbit);
  }

  _LIBCPP_HIDE_FROM_ABI bool __weekday_name(int& __result) {
    const time_get<_CharT>& __tg = use_facet<time_get<_CharT>>(__is_.getloc());
    istreambuf_iterator<_CharT, _Traits> __b(__sb_), __e;
    ios_base::iostate __err = ios_base::goodbit;
    tm __t{};
    __b = __tg.get_weekday(__b, __e, __is_, __err, &__t);
    if (__err & ios_base::eofbit)
      __state_ |= ios_base::eofbit;
    if (__err & ios_base::failbit)
      return false;
    __result = __t.tm_wday;
    return true;
  }

  _LIBCPP_HIDE_FROM_ABI bool __month_name(int& __result) {
    const time_get<_CharT>& __tg = use_facet<time_get<_CharT>>(__is_.getloc());
    istreambuf_iterator<_CharT, _Traits> __b(__sb_), __e;
    ios_base::iostate __err = ios_base::goodbit;
    tm __t{};
    __b = __tg.get_monthname(__b, __e, __is_, __err, &__t);
    if (__err & ios_base::eofbit)
      __state_ |= ios_base::eofbit;
    if (__err & ios_base::failbit)
      return false;
    __result = __t.tm_mon + 1;
    return true;
  }

  basic_istream<_CharT, _Traits>& __is_;
  basic_streambuf<_CharT, _Traits>* __sb_;
  const ctype<_CharT>& __ct_;
  const numpunct<_CharT>& __np_;
  ios_base::iostate __state_ = ios_base::goodbit;
};

// Parses the format string. __frac_width is the number of fractional-second digits of the destination type. Returns
// false when the input does not match; the caller then sets failbit.
template <class _CharT, class _Traits, class _Alloc>
_LIBCPP_HIDE_FROM_ABI bool __parse_format(
    __reader<_CharT, _Traits>& __r,
    const _CharT* __fmt,
    __parsed_fields& __f,
    basic_string<_CharT, _Traits, _Alloc>* __abbrev,
    bool __is_duration,
    unsigned __frac_width) {
  // A negative duration is written with a leading '-' before the first field.
  if (__is_duration && !__r.__at_eof() && __r.__narrow(__r.__peek()) == '-') {
    __r.__bump();
    __f.__negative_ = true;
  }

  auto __number = [&](int& __result, int __digits, int __width, bool __sign = false) -> bool {
    return __r.__read_int(__result, __width != 0 ? __width : __digits, __sign);
  };

  for (; *__fmt != _CharT(0); ++__fmt) {
    _CharT __c = *__fmt;
    if (__r.__is_space(__c)) {
      __r.__skip_whitespace();
      continue;
    }
    if (_Traits::eq(__c, __r.__widen('%')) == false) {
      if (!__r.__expect(__c))
        return false;
      continue;
    }

    ++__fmt;
    if (*__fmt == _CharT(0))
      return false; // dangling '%'

    int __width = 0;
    while (__r.__is_digit(*__fmt)) {
      __width = __width * 10 + (__r.__narrow(*__fmt) - '0');
      ++__fmt;
    }
    bool __modified = false; // E or O
    {
      char __n = __r.__narrow(*__fmt);
      if (__n == 'E' || __n == 'O') {
        __modified = true;
        ++__fmt;
      }
    }
    if (*__fmt == _CharT(0))
      return false;
    const char __spec = __r.__narrow(*__fmt);

    // Recursive expansion of the composite specifiers.
    auto __sub = [&](const char* __pattern) -> bool {
      _CharT __buffer[16];
      size_t __i = 0;
      for (; __pattern[__i] != '\0'; ++__i)
        __buffer[__i] = __r.__widen(__pattern[__i]);
      __buffer[__i] = _CharT(0);
      return __from_stream_detail::__parse_format(__r, __buffer, __f, __abbrev, __is_duration, __frac_width);
    };

    switch (__spec) {
    case 'a':
    case 'A': {
      int __wd;
      if (!__r.__weekday_name(__wd))
        return false;
      __f.__weekday_ = __wd;
      break;
    }
    case 'b':
    case 'B':
    case 'h': {
      int __m;
      if (!__r.__month_name(__m))
        return false;
      __f.__month_ = __m;
      break;
    }
    case 'c': {
      tm __t{};
      if (!__r.__time_get('c', __t))
        return false;
      __f.__year_ = __t.tm_year + 1900;
      __f.__has_year_ = true;
      __f.__month_ = __t.tm_mon + 1;
      __f.__day_ = __t.tm_mday;
      __f.__weekday_ = __t.tm_wday;
      __f.__hour24_ = __t.tm_hour;
      __f.__minute_ = __t.tm_min;
      __f.__second_ = __t.tm_sec;
      __f.__has_second_ = true;
      break;
    }
    case 'x': {
      tm __t{};
      if (!__r.__time_get('x', __t))
        return false;
      __f.__year_ = __t.tm_year + 1900;
      __f.__has_year_ = true;
      __f.__month_ = __t.tm_mon + 1;
      __f.__day_ = __t.tm_mday;
      break;
    }
    case 'X':
    case 'r': {
      tm __t{};
      if (!__r.__time_get(__spec, __t))
        return false;
      __f.__hour24_ = __t.tm_hour;
      __f.__minute_ = __t.tm_min;
      __f.__second_ = __t.tm_sec;
      __f.__has_second_ = true;
      break;
    }
    case 'C': {
      if (!__number(__f.__century_, 2, __width, true))
        return false;
      __f.__has_century_ = true;
      break;
    }
    case 'd':
    case 'e': {
      if (__r.__at_eof())
        return false;
      if (__spec == 'e' && __r.__is_space(__r.__peek()) && !__r.__is_digit(__r.__peek())) {
        // %e may be written with a leading blank for single digit days.
        __r.__bump();
      }
      if (!__number(__f.__day_, 2, __width))
        return false;
      break;
    }
    case 'D':
      if (!__sub("%m/%d/%y"))
        return false;
      break;
    case 'F': {
      // The width applies to %Y only.
      if (__width != 0) {
        if (!__number(__f.__year_, 4, __width, true))
          return false;
        __f.__has_year_ = true;
        if (!__sub("-%m-%d"))
          return false;
      } else if (!__sub("%Y-%m-%d")) {
        return false;
      }
      break;
    }
    case 'g':
      if (!__number(__f.__iso_yy_, 2, __width))
        return false;
      break;
    case 'G':
      if (!__number(__f.__iso_year_, 4, __width, true))
        return false;
      __f.__has_iso_year_ = true;
      break;
    case 'H':
      if (!__number(__f.__hour24_, 2, __width))
        return false;
      break;
    case 'I':
      if (!__number(__f.__hour12_, 2, __width))
        return false;
      break;
    case 'j':
      if (!__number(__f.__yday_, 3, __width))
        return false;
      break;
    case 'm':
      if (!__number(__f.__month_, 2, __width))
        return false;
      break;
    case 'M':
      if (!__number(__f.__minute_, 2, __width))
        return false;
      break;
    case 'n': {
      if (__r.__at_eof() || !__r.__is_space(__r.__peek()))
        return false;
      __r.__bump();
      break;
    }
    case 'p': {
      tm __t{};
      if (!__r.__time_get('p', __t))
        return false;
      __f.__ampm_ = __t.tm_hour >= 12 ? 1 : 0;
      break;
    }
    case 'R':
      if (!__sub("%H:%M"))
        return false;
      break;
    case 'S': {
      int __chars = __width != 0 ? __width : (__frac_width == 0 ? 2 : static_cast<int>(3 + __frac_width));
      long double __s;
      if (!__r.__read_seconds(__s, __chars))
        return false;
      __f.__second_ = __s;
      __f.__has_second_ = true;
      break;
    }
    case 't': {
      if (!__r.__at_eof() && __r.__is_space(__r.__peek()))
        __r.__bump();
      break;
    }
    case 'T': {
      if (__width != 0) {
        // The width belongs to the seconds field.
        if (!__sub("%H:%M:"))
          return false;
        int __chars = __width;
        long double __s;
        if (!__r.__read_seconds(__s, __chars))
          return false;
        __f.__second_ = __s;
        __f.__has_second_ = true;
      } else if (!__sub("%H:%M:%S")) {
        return false;
      }
      break;
    }
    case 'u': {
      int __u;
      if (!__number(__u, 1, __width))
        return false;
      if (__u < 1 || __u > 7)
        return false;
      __f.__weekday_ = __u % 7;
      break;
    }
    case 'U':
      if (!__number(__f.__week_sun_, 2, __width))
        return false;
      break;
    case 'V':
      if (!__number(__f.__iso_week_, 2, __width))
        return false;
      break;
    case 'w': {
      int __w;
      if (!__number(__w, 1, __width))
        return false;
      if (__w < 0 || __w > 6)
        return false;
      __f.__weekday_ = __w;
      break;
    }
    case 'W':
      if (!__number(__f.__week_mon_, 2, __width))
        return false;
      break;
    case 'y':
      if (!__number(__f.__yy_, 2, __width))
        return false;
      break;
    case 'Y':
      if (!__number(__f.__year_, 4, __width, true))
        return false;
      __f.__has_year_ = true;
      break;
    case 'z': {
      // [+|-]hh[mm]; with a modifier: [+|-]h[h][:mm]
      if (__r.__at_eof())
        return false;
      char __sign = __r.__narrow(__r.__peek());
      if (__sign != '+' && __sign != '-')
        return false;
      __r.__bump();
      int __hours = 0;
      if (!__r.__read_int(__hours, 2))
        return false;
      int __minutes = 0;
      if (__modified) {
        if (!__r.__at_eof() && __r.__narrow(__r.__peek()) == ':') {
          __r.__bump();
          if (!__r.__read_int(__minutes, 2))
            return false;
        }
      } else if (!__r.__at_eof() && __r.__is_digit(__r.__peek())) {
        if (!__r.__read_int(__minutes, 2))
          return false;
      }
      if (__hours > 23 || __minutes > 59)
        return false;
      int __total = __hours * 60 + __minutes;
      __f.__offset_minutes_ = __sign == '-' ? -__total : __total;
      __f.__has_offset_ = true;
      break;
    }
    case 'Z': {
      basic_string<_CharT, _Traits> __word;
      if (!__r.__read_zone_word(__word))
        return false;
      if (__abbrev != nullptr)
        __abbrev->assign(__word.data(), __word.size());
      break;
    }
    case '%':
      if (!__r.__expect_narrow('%'))
        return false;
      break;
    default:
      return false;
    }
  }
  return true;
}

inline constexpr int __days_in_common_month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

// Combines the year fields into a single year. Returns false if there is no year information.
_LIBCPP_HIDE_FROM_ABI inline bool __combine_year(const __parsed_fields& __f, int& __year) {
  if (__f.__yy_ != -1) {
    if (__f.__yy_ > 99)
      return false;
    if (__f.__has_century_)
      __year = __f.__century_ >= 0 ? __f.__century_ * 100 + __f.__yy_ : __f.__century_ * 100 - __f.__yy_;
    else
      __year = __f.__yy_ >= 69 ? 1900 + __f.__yy_ : 2000 + __f.__yy_;
    if (__f.__has_year_ && __f.__year_ != __year)
      return false;
    return true;
  }
  if (__f.__has_year_) {
    __year = __f.__year_;
    if (__f.__has_century_ && __f.__century_ != (__year >= 0 ? __year / 100 : -((-__year) / 100)))
      return false;
    return true;
  }
  return false;
}

_LIBCPP_HIDE_FROM_ABI inline bool __combine_iso_year(const __parsed_fields& __f, int& __year) {
  if (__f.__iso_yy_ != -1) {
    if (__f.__iso_yy_ > 99)
      return false;
    if (__f.__has_century_)
      __year = __f.__century_ * 100 + __f.__iso_yy_;
    else
      __year = __f.__iso_yy_ >= 69 ? 1900 + __f.__iso_yy_ : 2000 + __f.__iso_yy_;
    return true;
  }
  if (__f.__has_iso_year_) {
    __year = __f.__iso_year_;
    return true;
  }
  return false;
}

// Builds a complete date from the parsed fields. Returns false if the fields don't describe a valid date.
_LIBCPP_HIDE_FROM_ABI inline bool __to_year_month_day(const __parsed_fields& __f, year_month_day& __result) {
  int __year;
  const bool __has_civil_year = __combine_year(__f, __year);

  sys_days __days{};
  bool __have_date = false;

  if (__has_civil_year && __f.__month_ != -1 && __f.__day_ != -1) {
    year_month_day __ymd{year{__year}, month{static_cast<unsigned>(__f.__month_)}, day{static_cast<unsigned>(__f.__day_)}};
    if (__f.__month_ < 1 || __f.__month_ > 12 || __f.__day_ < 1 || !__ymd.ok())
      return false;
    __days      = sys_days{__ymd};
    __have_date = true;
  } else if (__has_civil_year && __f.__yday_ != -1) {
    year __y{__year};
    if (!__y.ok() || __f.__yday_ < 1 || __f.__yday_ > (__y.is_leap() ? 366 : 365))
      return false;
    __days      = sys_days{__y / January / 1} + days{__f.__yday_ - 1};
    __have_date = true;
  } else if (__f.__iso_week_ != -1) {
    int __iso_year;
    if (!__combine_iso_year(__f, __iso_year) || __f.__weekday_ == -1 || __f.__iso_week_ < 1 || __f.__iso_week_ > 53)
      return false;
    // ISO 8601 week 1 contains January 4th; weeks start on Monday.
    sys_days __jan4      = sys_days{year{__iso_year} / January / 4};
    weekday __jan4_wd    = weekday{__jan4};
    int __jan4_mon_based = static_cast<int>((__jan4_wd - Monday).count()); // 0..6
    sys_days __week1     = __jan4 - days{__jan4_mon_based};
    int __day_in_week    = (__f.__weekday_ + 6) % 7; // Monday == 0
    __days               = __week1 + days{(__f.__iso_week_ - 1) * 7 + __day_in_week};
    // Week 53 only exists in "long" ISO years.
    sys_days __next_week1 = sys_days{year{__iso_year + 1} / January / 4};
    weekday __next_wd     = weekday{__next_week1};
    __next_week1 -= days{static_cast<int>((__next_wd - Monday).count())};
    if (__f.__iso_week_ == 53 && !(__week1 + days{52 * 7} + days{__day_in_week} < __next_week1))
      return false;
    __have_date = true;
  } else if (__has_civil_year && (__f.__week_sun_ != -1 || __f.__week_mon_ != -1) && __f.__weekday_ != -1) {
    // Week numbers where the first Sunday (%U) or Monday (%W) starts week 1.
    year __y{__year};
    if (!__y.ok())
      return false;
    sys_days __jan1 = sys_days{__y / January / 1};
    int __week;
    weekday __first;
    if (__f.__week_sun_ != -1) {
      __week = __f.__week_sun_;
      __first = Sunday;
    } else {
      __week = __f.__week_mon_;
      __first = Monday;
    }
    if (__week < 0 || __week > 53)
      return false;
    int __jan1_offset = static_cast<int>((weekday{__jan1} - __first).count()); // days since the week start, 0..6
    // Day index of the first day of week 1 relative to Jan 1.
    int __week1_start = __jan1_offset == 0 ? 0 : 7 - __jan1_offset;
    int __wd_offset   = static_cast<int>((weekday{static_cast<unsigned>(__f.__weekday_)} - __first).count());
    __days            = __jan1 + days{__week1_start + (__week - 1) * 7 + __wd_offset};
    __have_date       = true;
    if (year_month_day{__days}.year() != __y)
      return false;
  }

  if (!__have_date)
    return false;

  __result = year_month_day{__days};
  if (!__result.ok())
    return false;
  // A weekday that was parsed as well must agree with the date.
  if (__f.__weekday_ != -1 && weekday{__days}.c_encoding() != static_cast<unsigned>(__f.__weekday_))
    return false;
  return true;
}

// Time of day as a number of seconds (possibly fractional). __allow_leap permits a seconds field of 60.
_LIBCPP_HIDE_FROM_ABI inline bool
__to_seconds_of_day(const __parsed_fields& __f, bool __allow_leap, long double& __result) {
  int __h = 0;
  if (__f.__hour24_ != -1) {
    if (__f.__hour24_ > 23)
      return false;
    __h = __f.__hour24_;
    if (__f.__hour12_ != -1) {
      // Both given: they must agree.
      int __h12 = __f.__hour12_ % 12 + (__f.__ampm_ == 1 ? 12 : 0);
      if (__f.__hour12_ < 1 || __f.__hour12_ > 12 || __h12 != __h)
        return false;
    }
  } else if (__f.__hour12_ != -1) {
    if (__f.__hour12_ < 1 || __f.__hour12_ > 12)
      return false;
    __h = __f.__hour12_ % 12 + (__f.__ampm_ == 1 ? 12 : 0);
  }
  int __m = __f.__minute_ == -1 ? 0 : __f.__minute_;
  if (__m > 59)
    return false;
  long double __s = __f.__has_second_ ? __f.__second_ : 0;
  if (__s < 0 || __s >= (__allow_leap ? 61 : 60))
    return false;
  __result = __h * 3600.0L + __m * 60.0L + __s;
  return true;
}

// Converts seconds (long double) to a duration of type _Duration; fails if the value isn't representable.
template <class _Duration>
_LIBCPP_HIDE_FROM_ABI bool __seconds_to_duration(long double __seconds, _Duration& __result) {
  using _Rep    = typename _Duration::rep;
  using _Period = typename _Duration::period;
  long double __units = __seconds * _Period::den / _Period::num;
  if constexpr (treat_as_floating_point_v<_Rep>) {
    __result = _Duration{static_cast<_Rep>(__units)};
    return true;
  } else {
    long double __rounded = std::nearbyint(__units);
    if (std::fabs(__units - __rounded) > 1e-6L * (1 + std::fabs(__rounded)))
      return false; // the value has a finer resolution than _Duration can represent
    if (std::fabs(__rounded) > static_cast<long double>(numeric_limits<_Rep>::max()))
      return false;
    __result = _Duration{static_cast<_Rep>(__rounded)};
    return true;
  }
}

template <class _Duration>
_LIBCPP_HIDE_FROM_ABI constexpr unsigned __fractional_width_of() {
  if constexpr (treat_as_floating_point_v<typename _Duration::rep>)
    return 6;
  else
    return hh_mm_ss<common_type_t<_Duration, seconds>>::fractional_width;
}

// The shared driver. __convert receives the parsed fields and stores the value, returning false when the fields
// don't describe a value of the destination type.
template <class _CharT, class _Traits, class _Alloc, class _Convert>
_LIBCPP_HIDE_FROM_ABI basic_istream<_CharT, _Traits>& __from_stream_impl(
    basic_istream<_CharT, _Traits>& __is,
    const _CharT* __fmt,
    basic_string<_CharT, _Traits, _Alloc>* __abbrev,
    minutes* __offset,
    bool __is_duration,
    unsigned __frac_width,
    _Convert&& __convert) {
  typename basic_istream<_CharT, _Traits>::sentry __s(__is, true);
  if (!__s)
    return __is;

  ios_base::iostate __state = ios_base::goodbit;
#    if _LIBCPP_HAS_EXCEPTIONS
  try {
#    endif
    __reader<_CharT, _Traits> __r(__is);
    __parsed_fields __f;
    basic_string<_CharT, _Traits, _Alloc> __abbrev_value;
    bool __ok = __from_stream_detail::__parse_format(
        __r, __fmt, __f, __abbrev != nullptr ? &__abbrev_value : nullptr, __is_duration, __frac_width);
    __state |= __r.__state_;
    if (__ok)
      __ok = __convert(__f);
    if (!__ok) {
      __state |= ios_base::failbit;
    } else {
      if (__abbrev != nullptr && !__abbrev_value.empty())
        *__abbrev = std::move(__abbrev_value);
      if (__offset != nullptr && __f.__has_offset_)
        *__offset = minutes{__f.__offset_minutes_};
    }
#    if _LIBCPP_HAS_EXCEPTIONS
  } catch (...) {
    __state |= ios_base::badbit;
    __is.__setstate_nothrow(__state);
    if (__is.exceptions() & ios_base::badbit)
      throw;
    return __is;
  }
#    endif
  __is.setstate(__state);
  return __is;
}

} // namespace __from_stream_detail

// duration
template <class _CharT, class _Traits, class _Rep, class _Period, class _Alloc = allocator<_CharT>>
_LIBCPP_HIDE_FROM_ABI basic_istream<_CharT, _Traits>& from_stream(
    basic_istream<_CharT, _Traits>& __is,
    const _CharT* __fmt,
    duration<_Rep, _Period>& __d,
    basic_string<_CharT, _Traits, _Alloc>* __abbrev = nullptr,
    minutes* __offset                               = nullptr) {
  using _Dur = duration<_Rep, _Period>;
  return __from_stream_detail::__from_stream_impl(
      __is,
      __fmt,
      __abbrev,
      __offset,
      /*__is_duration=*/true,
      __from_stream_detail::__fractional_width_of<_Dur>(),
      [&](const __from_stream_detail::__parsed_fields& __f) -> bool {
        if (__f.__has_year_ || __f.__has_century_ || __f.__yy_ != -1 || __f.__has_iso_year_ ||
            __f.__iso_yy_ != -1 || __f.__month_ != -1 || __f.__day_ != -1 || __f.__week_sun_ != -1 ||
            __f.__week_mon_ != -1 || __f.__iso_week_ != -1 || __f.__weekday_ != -1)
          return false;
        long double __seconds;
        if (!__from_stream_detail::__to_seconds_of_day(__f, false, __seconds))
          return false;
        if (!__f.__has_time_fields() && __f.__yday_ == -1)
          return false;
        if (__f.__yday_ != -1)
          __seconds += __f.__yday_ * 86400.0L;
        if (__f.__negative_)
          __seconds = -__seconds;
        _Dur __value;
        if (!__from_stream_detail::__seconds_to_duration(__seconds, __value))
          return false;
        __d = __value;
        return true;
      });
}

// day
template <class _CharT, class _Traits, class _Alloc = allocator<_CharT>>
_LIBCPP_HIDE_FROM_ABI basic_istream<_CharT, _Traits>& from_stream(
    basic_istream<_CharT, _Traits>& __is,
    const _CharT* __fmt,
    day& __d,
    basic_string<_CharT, _Traits, _Alloc>* __abbrev = nullptr,
    minutes* __offset                               = nullptr) {
  return __from_stream_detail::__from_stream_impl(
      __is, __fmt, __abbrev, __offset, false, 0, [&](const __from_stream_detail::__parsed_fields& __f) -> bool {
        if (__f.__day_ == -1)
          return false;
        day __value{static_cast<unsigned>(__f.__day_)};
        if (!__value.ok())
          return false;
        __d = __value;
        return true;
      });
}

// month
template <class _CharT, class _Traits, class _Alloc = allocator<_CharT>>
_LIBCPP_HIDE_FROM_ABI basic_istream<_CharT, _Traits>& from_stream(
    basic_istream<_CharT, _Traits>& __is,
    const _CharT* __fmt,
    month& __m,
    basic_string<_CharT, _Traits, _Alloc>* __abbrev = nullptr,
    minutes* __offset                               = nullptr) {
  return __from_stream_detail::__from_stream_impl(
      __is, __fmt, __abbrev, __offset, false, 0, [&](const __from_stream_detail::__parsed_fields& __f) -> bool {
        if (__f.__month_ == -1)
          return false;
        month __value{static_cast<unsigned>(__f.__month_)};
        if (!__value.ok())
          return false;
        __m = __value;
        return true;
      });
}

// year
template <class _CharT, class _Traits, class _Alloc = allocator<_CharT>>
_LIBCPP_HIDE_FROM_ABI basic_istream<_CharT, _Traits>& from_stream(
    basic_istream<_CharT, _Traits>& __is,
    const _CharT* __fmt,
    year& __y,
    basic_string<_CharT, _Traits, _Alloc>* __abbrev = nullptr,
    minutes* __offset                               = nullptr) {
  return __from_stream_detail::__from_stream_impl(
      __is, __fmt, __abbrev, __offset, false, 0, [&](const __from_stream_detail::__parsed_fields& __f) -> bool {
        int __value;
        if (!__from_stream_detail::__combine_year(__f, __value))
          return false;
        year __candidate{__value};
        if (!__candidate.ok())
          return false;
        __y = __candidate;
        return true;
      });
}

// weekday
template <class _CharT, class _Traits, class _Alloc = allocator<_CharT>>
_LIBCPP_HIDE_FROM_ABI basic_istream<_CharT, _Traits>& from_stream(
    basic_istream<_CharT, _Traits>& __is,
    const _CharT* __fmt,
    weekday& __wd,
    basic_string<_CharT, _Traits, _Alloc>* __abbrev = nullptr,
    minutes* __offset                               = nullptr) {
  return __from_stream_detail::__from_stream_impl(
      __is, __fmt, __abbrev, __offset, false, 0, [&](const __from_stream_detail::__parsed_fields& __f) -> bool {
        if (__f.__weekday_ == -1)
          return false;
        __wd = weekday{static_cast<unsigned>(__f.__weekday_)};
        return true;
      });
}

// month_day
template <class _CharT, class _Traits, class _Alloc = allocator<_CharT>>
_LIBCPP_HIDE_FROM_ABI basic_istream<_CharT, _Traits>& from_stream(
    basic_istream<_CharT, _Traits>& __is,
    const _CharT* __fmt,
    month_day& __md,
    basic_string<_CharT, _Traits, _Alloc>* __abbrev = nullptr,
    minutes* __offset                               = nullptr) {
  return __from_stream_detail::__from_stream_impl(
      __is, __fmt, __abbrev, __offset, false, 0, [&](const __from_stream_detail::__parsed_fields& __f) -> bool {
        if (__f.__month_ == -1 || __f.__day_ == -1)
          return false;
        month_day __value{month{static_cast<unsigned>(__f.__month_)}, day{static_cast<unsigned>(__f.__day_)}};
        if (!__value.ok())
          return false;
        __md = __value;
        return true;
      });
}

// year_month
template <class _CharT, class _Traits, class _Alloc = allocator<_CharT>>
_LIBCPP_HIDE_FROM_ABI basic_istream<_CharT, _Traits>& from_stream(
    basic_istream<_CharT, _Traits>& __is,
    const _CharT* __fmt,
    year_month& __ym,
    basic_string<_CharT, _Traits, _Alloc>* __abbrev = nullptr,
    minutes* __offset                               = nullptr) {
  return __from_stream_detail::__from_stream_impl(
      __is, __fmt, __abbrev, __offset, false, 0, [&](const __from_stream_detail::__parsed_fields& __f) -> bool {
        int __y;
        if (!__from_stream_detail::__combine_year(__f, __y) || __f.__month_ == -1)
          return false;
        year_month __value{year{__y}, month{static_cast<unsigned>(__f.__month_)}};
        if (!__value.ok())
          return false;
        __ym = __value;
        return true;
      });
}

// year_month_day
template <class _CharT, class _Traits, class _Alloc = allocator<_CharT>>
_LIBCPP_HIDE_FROM_ABI basic_istream<_CharT, _Traits>& from_stream(
    basic_istream<_CharT, _Traits>& __is,
    const _CharT* __fmt,
    year_month_day& __ymd,
    basic_string<_CharT, _Traits, _Alloc>* __abbrev = nullptr,
    minutes* __offset                               = nullptr) {
  return __from_stream_detail::__from_stream_impl(
      __is, __fmt, __abbrev, __offset, false, 0, [&](const __from_stream_detail::__parsed_fields& __f) -> bool {
        year_month_day __value;
        if (!__from_stream_detail::__to_year_month_day(__f, __value))
          return false;
        __ymd = __value;
        return true;
      });
}

namespace __from_stream_detail {

// Shared conversion for the time_point flavours: the date and time of day as a duration since the epoch of the
// system clock, with the parsed UTC offset already applied when __apply_offset is set.
template <class _Duration>
_LIBCPP_HIDE_FROM_ABI bool
__to_sys_time(const __parsed_fields& __f, bool __apply_offset, bool __allow_leap, sys_time<_Duration>& __result, bool& __leap) {
  year_month_day __ymd;
  if (!__from_stream_detail::__to_year_month_day(__f, __ymd))
    return false;
  long double __seconds;
  if (!__from_stream_detail::__to_seconds_of_day(__f, __allow_leap, __seconds))
    return false;
  __leap = false;
  if (__f.__has_second_ && __f.__second_ >= 60) {
    // A leap second: represented as 23:59:59 plus one second by the caller.
    __leap = true;
    __seconds -= 1;
  }
  if (__apply_offset && __f.__has_offset_)
    __seconds -= __f.__offset_minutes_ * 60.0L;
  _Duration __tod;
  if (!__from_stream_detail::__seconds_to_duration(__seconds, __tod))
    return false;
  __result = sys_time<_Duration>{duration_cast<_Duration>(sys_days{__ymd}.time_since_epoch()) + __tod};
  return true;
}

} // namespace __from_stream_detail

// sys_time
template <class _CharT, class _Traits, class _Duration, class _Alloc = allocator<_CharT>>
_LIBCPP_HIDE_FROM_ABI basic_istream<_CharT, _Traits>& from_stream(
    basic_istream<_CharT, _Traits>& __is,
    const _CharT* __fmt,
    sys_time<_Duration>& __tp,
    basic_string<_CharT, _Traits, _Alloc>* __abbrev = nullptr,
    minutes* __offset                               = nullptr) {
  return __from_stream_detail::__from_stream_impl(
      __is,
      __fmt,
      __abbrev,
      __offset,
      false,
      __from_stream_detail::__fractional_width_of<_Duration>(),
      [&](const __from_stream_detail::__parsed_fields& __f) -> bool {
        sys_time<_Duration> __value;
        bool __leap;
        if (!__from_stream_detail::__to_sys_time(__f, /*__apply_offset=*/true, /*__allow_leap=*/false, __value, __leap))
          return false;
        __tp = __value;
        return true;
      });
}

// local_time
template <class _CharT, class _Traits, class _Duration, class _Alloc = allocator<_CharT>>
_LIBCPP_HIDE_FROM_ABI basic_istream<_CharT, _Traits>& from_stream(
    basic_istream<_CharT, _Traits>& __is,
    const _CharT* __fmt,
    local_time<_Duration>& __tp,
    basic_string<_CharT, _Traits, _Alloc>* __abbrev = nullptr,
    minutes* __offset                               = nullptr) {
  return __from_stream_detail::__from_stream_impl(
      __is,
      __fmt,
      __abbrev,
      __offset,
      false,
      __from_stream_detail::__fractional_width_of<_Duration>(),
      [&](const __from_stream_detail::__parsed_fields& __f) -> bool {
        sys_time<_Duration> __value;
        bool __leap;
        if (!__from_stream_detail::__to_sys_time(__f, /*__apply_offset=*/false, /*__allow_leap=*/false, __value, __leap))
          return false;
        __tp = local_time<_Duration>{__value.time_since_epoch()};
        return true;
      });
}

// file_time
template <class _CharT, class _Traits, class _Duration, class _Alloc = allocator<_CharT>>
_LIBCPP_HIDE_FROM_ABI basic_istream<_CharT, _Traits>& from_stream(
    basic_istream<_CharT, _Traits>& __is,
    const _CharT* __fmt,
    file_time<_Duration>& __tp,
    basic_string<_CharT, _Traits, _Alloc>* __abbrev = nullptr,
    minutes* __offset                               = nullptr) {
  sys_time<_Duration> __sys;
  chrono::from_stream(__is, __fmt, __sys, __abbrev, __offset);
  if (!__is.fail())
    __tp = file_clock::from_sys(__sys);
  return __is;
}

#    if _LIBCPP_HAS_TIME_ZONE_DATABASE && _LIBCPP_HAS_FILESYSTEM && _LIBCPP_HAS_EXPERIMENTAL_TZDB

// utc_time: a seconds field of 60 is accepted when the parsed instant is a leap second.
template <class _CharT, class _Traits, class _Duration, class _Alloc = allocator<_CharT>>
_LIBCPP_HIDE_FROM_ABI basic_istream<_CharT, _Traits>& from_stream(
    basic_istream<_CharT, _Traits>& __is,
    const _CharT* __fmt,
    utc_time<_Duration>& __tp,
    basic_string<_CharT, _Traits, _Alloc>* __abbrev = nullptr,
    minutes* __offset                               = nullptr) {
  return __from_stream_detail::__from_stream_impl(
      __is,
      __fmt,
      __abbrev,
      __offset,
      false,
      __from_stream_detail::__fractional_width_of<_Duration>(),
      [&](const __from_stream_detail::__parsed_fields& __f) -> bool {
        sys_time<_Duration> __sys;
        bool __leap;
        if (!__from_stream_detail::__to_sys_time(__f, /*__apply_offset=*/true, /*__allow_leap=*/true, __sys, __leap))
          return false;
        utc_time<_Duration> __value = utc_clock::from_sys(__sys);
        if (__leap) {
          // 23:59:60 only exists if the day ends with a leap second.
          utc_time<_Duration> __plus = __value + duration_cast<_Duration>(seconds{1});
          if (!chrono::get_leap_second_info(__plus).is_leap_second)
            return false;
          __value = __plus;
        }
        __tp = __value;
        return true;
      });
}

// tai_time
template <class _CharT, class _Traits, class _Duration, class _Alloc = allocator<_CharT>>
_LIBCPP_HIDE_FROM_ABI basic_istream<_CharT, _Traits>& from_stream(
    basic_istream<_CharT, _Traits>& __is,
    const _CharT* __fmt,
    tai_time<_Duration>& __tp,
    basic_string<_CharT, _Traits, _Alloc>* __abbrev = nullptr,
    minutes* __offset                               = nullptr) {
  utc_time<common_type_t<_Duration, seconds>> __utc;
  chrono::from_stream(__is, __fmt, __utc, __abbrev, __offset);
  if (!__is.fail())
    __tp = tai_clock::from_utc(__utc);
  return __is;
}

// gps_time
template <class _CharT, class _Traits, class _Duration, class _Alloc = allocator<_CharT>>
_LIBCPP_HIDE_FROM_ABI basic_istream<_CharT, _Traits>& from_stream(
    basic_istream<_CharT, _Traits>& __is,
    const _CharT* __fmt,
    gps_time<_Duration>& __tp,
    basic_string<_CharT, _Traits, _Alloc>* __abbrev = nullptr,
    minutes* __offset                               = nullptr) {
  utc_time<common_type_t<_Duration, seconds>> __utc;
  chrono::from_stream(__is, __fmt, __utc, __abbrev, __offset);
  if (!__is.fail())
    __tp = gps_clock::from_utc(__utc);
  return __is;
}

#    endif // _LIBCPP_HAS_TIME_ZONE_DATABASE && _LIBCPP_HAS_FILESYSTEM && _LIBCPP_HAS_EXPERIMENTAL_TZDB

// [time.parse], the parse manipulators.

namespace __from_stream_detail {

template <class _CharT, class _Parsable, class _AbbrevString, bool _HasAbbrev, bool _HasOffset>
class __parse_manip {
public:
  _LIBCPP_HIDE_FROM_ABI __parse_manip(const _CharT* __fmt, _Parsable& __tp, _AbbrevString* __abbrev, minutes* __offset)
      : __fmt_(__fmt), __tp_(__tp), __abbrev_(__abbrev), __offset_(__offset) {}

  // The manipulator holds references to a format string and to the destination; make it hard to keep one around.
  __parse_manip(const __parse_manip&)            = delete;
  __parse_manip& operator=(const __parse_manip&) = delete;

  template <class _Traits>
  _LIBCPP_HIDE_FROM_ABI friend basic_istream<_CharT, _Traits>&
  operator>>(basic_istream<_CharT, _Traits>& __is, __parse_manip&& __m) {
    if constexpr (_HasAbbrev && _HasOffset)
      return from_stream(__is, __m.__fmt_, __m.__tp_, __m.__abbrev_, __m.__offset_);
    else if constexpr (_HasAbbrev)
      return from_stream(__is, __m.__fmt_, __m.__tp_, __m.__abbrev_);
    else if constexpr (_HasOffset)
      return from_stream(__is, __m.__fmt_, __m.__tp_, static_cast<_AbbrevString*>(nullptr), __m.__offset_);
    else
      return from_stream(__is, __m.__fmt_, __m.__tp_);
  }

private:
  const _CharT* __fmt_;
  _Parsable& __tp_;
  _AbbrevString* __abbrev_;
  minutes* __offset_;
};

} // namespace __from_stream_detail

template <class _CharT, class _Parsable>
  requires requires(basic_istream<_CharT, char_traits<_CharT>>& __is, const _CharT* __f, _Parsable& __tp) {
    from_stream(__is, __f, __tp);
  }
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI auto parse(const _CharT* __fmt, _Parsable& __tp) {
  return __from_stream_detail::__parse_manip<_CharT, _Parsable, basic_string<_CharT>, false, false>(
      __fmt, __tp, nullptr, nullptr);
}

template <class _CharT, class _Traits, class _Alloc, class _Parsable>
  requires requires(basic_istream<_CharT, _Traits>& __is, const _CharT* __f, _Parsable& __tp) {
    from_stream(__is, __f, __tp);
  }
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI auto parse(const basic_string<_CharT, _Traits, _Alloc>& __fmt, _Parsable& __tp) {
  return __from_stream_detail::__parse_manip<_CharT, _Parsable, basic_string<_CharT, _Traits, _Alloc>, false, false>(
      __fmt.c_str(), __tp, nullptr, nullptr);
}

template <class _CharT, class _Traits, class _Alloc, class _Parsable>
  requires requires(basic_istream<_CharT, _Traits>& __is,
                    const _CharT* __f,
                    _Parsable& __tp,
                    basic_string<_CharT, _Traits, _Alloc>& __abbrev) { from_stream(__is, __f, __tp, &__abbrev); }
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI auto
parse(const _CharT* __fmt, _Parsable& __tp, basic_string<_CharT, _Traits, _Alloc>& __abbrev) {
  return __from_stream_detail::__parse_manip<_CharT, _Parsable, basic_string<_CharT, _Traits, _Alloc>, true, false>(
      __fmt, __tp, std::addressof(__abbrev), nullptr);
}

template <class _CharT, class _Traits, class _Alloc, class _Parsable>
  requires requires(basic_istream<_CharT, _Traits>& __is,
                    const _CharT* __f,
                    _Parsable& __tp,
                    basic_string<_CharT, _Traits, _Alloc>& __abbrev) { from_stream(__is, __f, __tp, &__abbrev); }
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI auto parse(
    const basic_string<_CharT, _Traits, _Alloc>& __fmt,
    _Parsable& __tp,
    basic_string<_CharT, _Traits, _Alloc>& __abbrev) {
  return __from_stream_detail::__parse_manip<_CharT, _Parsable, basic_string<_CharT, _Traits, _Alloc>, true, false>(
      __fmt.c_str(), __tp, std::addressof(__abbrev), nullptr);
}

template <class _CharT, class _Parsable>
  requires requires(basic_istream<_CharT, char_traits<_CharT>>& __is,
                    const _CharT* __f,
                    _Parsable& __tp,
                    minutes& __offset) {
    from_stream(__is, __f, __tp, static_cast<basic_string<_CharT>*>(nullptr), &__offset);
  }
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI auto parse(const _CharT* __fmt, _Parsable& __tp, minutes& __offset) {
  return __from_stream_detail::__parse_manip<_CharT, _Parsable, basic_string<_CharT>, false, true>(
      __fmt, __tp, nullptr, std::addressof(__offset));
}

template <class _CharT, class _Traits, class _Alloc, class _Parsable>
  requires requires(basic_istream<_CharT, _Traits>& __is,
                    const _CharT* __f,
                    _Parsable& __tp,
                    minutes& __offset) {
    from_stream(__is, __f, __tp, static_cast<basic_string<_CharT, _Traits, _Alloc>*>(nullptr), &__offset);
  }
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI auto
parse(const basic_string<_CharT, _Traits, _Alloc>& __fmt, _Parsable& __tp, minutes& __offset) {
  return __from_stream_detail::__parse_manip<_CharT, _Parsable, basic_string<_CharT, _Traits, _Alloc>, false, true>(
      __fmt.c_str(), __tp, nullptr, std::addressof(__offset));
}

template <class _CharT, class _Traits, class _Alloc, class _Parsable>
  requires requires(basic_istream<_CharT, _Traits>& __is,
                    const _CharT* __f,
                    _Parsable& __tp,
                    basic_string<_CharT, _Traits, _Alloc>& __abbrev,
                    minutes& __offset) { from_stream(__is, __f, __tp, &__abbrev, &__offset); }
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI auto parse(
    const _CharT* __fmt, _Parsable& __tp, basic_string<_CharT, _Traits, _Alloc>& __abbrev, minutes& __offset) {
  return __from_stream_detail::__parse_manip<_CharT, _Parsable, basic_string<_CharT, _Traits, _Alloc>, true, true>(
      __fmt, __tp, std::addressof(__abbrev), std::addressof(__offset));
}

template <class _CharT, class _Traits, class _Alloc, class _Parsable>
  requires requires(basic_istream<_CharT, _Traits>& __is,
                    const _CharT* __f,
                    _Parsable& __tp,
                    basic_string<_CharT, _Traits, _Alloc>& __abbrev,
                    minutes& __offset) { from_stream(__is, __f, __tp, &__abbrev, &__offset); }
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI auto parse(
    const basic_string<_CharT, _Traits, _Alloc>& __fmt,
    _Parsable& __tp,
    basic_string<_CharT, _Traits, _Alloc>& __abbrev,
    minutes& __offset) {
  return __from_stream_detail::__parse_manip<_CharT, _Parsable, basic_string<_CharT, _Traits, _Alloc>, true, true>(
      __fmt.c_str(), __tp, std::addressof(__abbrev), std::addressof(__offset));
}

} // namespace chrono

#  endif // _LIBCPP_STD_VER >= 20

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP_HAS_LOCALIZATION

#endif // _LIBCPP___CHRONO_FROM_STREAM_H
