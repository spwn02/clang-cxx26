//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17
// UNSUPPORTED: no-localization

// <chrono>

// [time.parse]: from_stream for the calendar types, durations and time_points.

#include <cassert>
#include <chrono>
#include <sstream>
#include <string>

#include "test_macros.h"

using namespace std::chrono;
using namespace std::literals::chrono_literals;

template <class T>
bool parse_ok(const char* input, const char* fmt, T& value) {
  std::istringstream is(input);
  std::chrono::from_stream(is, fmt, value);
  return !is.fail();
}

static void test_day_month_year_weekday() {
  {
    day d{1};
    assert(parse_ok("17", "%d", d) && d == day{17});
    assert(parse_ok(" 5", "%e", d) && d == day{5});
    assert(parse_ok("09", "%Od", d) && d == day{9}); // LWG 3218: %Od (and %Oe) are the POSIX spellings
    assert(parse_ok("31", "%Oe", d) && d == day{31});
    day before{3};
    std::istringstream bad("32");
    from_stream(bad, "%d", before);
    assert(bad.fail() && before == day{3}); // failure leaves the object unmodified (LWG 3536)
    std::istringstream nonsense("xx");
    from_stream(nonsense, "%d", before);
    assert(nonsense.fail() && before == day{3});
  }
  {
    month m{1};
    assert(parse_ok("12", "%m", m) && m == December);
    assert(parse_ok("Mar", "%b", m) && m == March);
    assert(parse_ok("march", "%B", m) && m == March); // case-insensitive full or abbreviated names
    assert(parse_ok("AUG", "%h", m) && m == August);
    month before{2};
    std::istringstream bad("13");
    from_stream(bad, "%m", before);
    assert(bad.fail() && before == February);
  }
  {
    year y{0};
    assert(parse_ok("2024", "%Y", y) && y == year{2024});
    assert(parse_ok("-0044", "%Y", y) && y == year{-44});
    assert(parse_ok("68", "%y", y) && y == year{2068}); // [00,68] -> 20xx
    assert(parse_ok("69", "%y", y) && y == year{1969}); // [69,99] -> 19xx
    assert(parse_ok("20 15", "%C %y", y) && y == year{2015});
    assert(parse_ok("12345", "%5Y", y) && y == year{12345}); // width
    year before{7};
    std::istringstream bad("abcd");
    from_stream(bad, "%Y", before);
    assert(bad.fail() && before == year{7});
  }
  {
    weekday wd{0};
    assert(parse_ok("Tue", "%a", wd) && wd == Tuesday);
    assert(parse_ok("friday", "%A", wd) && wd == Friday);
    assert(parse_ok("0", "%w", wd) && wd == Sunday);
    assert(parse_ok("7", "%u", wd) && wd == Sunday);
    assert(parse_ok("1", "%u", wd) && wd == Monday);
    weekday before{Monday};
    std::istringstream bad("8");
    from_stream(bad, "%u", before);
    assert(bad.fail() && before == Monday);
  }
}

static void test_composites() {
  {
    month_day md{January / 1};
    assert(parse_ok("02-29", "%m-%d", md) && md == February / 29);
    std::istringstream bad("02-30");
    from_stream(bad, "%m-%d", md);
    assert(bad.fail() && md == February / 29);
    std::istringstream missing("02");
    from_stream(missing, "%m", md); // a month_day needs both the month and the day
    assert(missing.fail());
  }
  {
    year_month ym{year{1} / January};
    assert(parse_ok("2020/07", "%Y/%m", ym) && ym == year{2020} / July);
    std::istringstream missing("2020");
    from_stream(missing, "%Y", ym);
    assert(missing.fail());
  }
  {
    year_month_day ymd{};
    assert(parse_ok("2020-02-29", "%F", ymd) && ymd == year{2020} / February / 29);
    assert(parse_ok("12/31/99", "%D", ymd) && ymd == year{1999} / December / 31);
    assert(parse_ok("2021 065", "%Y %j", ymd) && ymd == year{2021} / March / 6);
    assert(parse_ok("2021 366", "%Y %j", ymd) == false);
    assert(parse_ok("2020 366", "%Y %j", ymd) && ymd == year{2020} / December / 31);
    assert(parse_ok("2021-W10-3", "%G-W%V-%u", ymd) && ymd == year{2021} / March / 10);
    assert(parse_ok("2020-W53-7", "%G-W%V-%u", ymd) && ymd == year{2021} / January / 3);
    assert(parse_ok("2021 09 Mon", "%Y %U %a", ymd) && ymd == year{2021} / March / 1); // week 9 (Sunday based), Monday
    assert(parse_ok("2021 09 Mon", "%Y %W %a", ymd) && ymd == year{2021} / March / 1);
    assert(parse_ok("Tue 2021-03-02", "%a %F", ymd) && ymd == year{2021} / March / 2);
    // a weekday that disagrees with the date is an error
    assert(parse_ok("Mon 2021-03-02", "%a %F", ymd) == false);
    assert(parse_ok("2021-02-29", "%F", ymd) == false);
    assert(parse_ok("2021-03", "%F", ymd) == false);
    assert(parse_ok("2021", "%Y", ymd) == false);
  }
}

static void test_literals_and_whitespace() {
  year_month_day ymd{};
  assert(parse_ok("2020-02-29", "%Y-%m-%d", ymd));
  assert(parse_ok("2020 - 02 -   29", "%Y - %m - %d", ymd) && ymd == year{2020} / February / 29);
  assert(parse_ok("2020-02-29", "%Y - %m - %d", ymd) && ymd == year{2020} / February / 29); // blank == zero or more
  assert(parse_ok("2020\n\t02   29", "%Y %m %d", ymd));
  assert(parse_ok("2020  02", "%Y%n%m %d", ymd) == false); // %n is exactly one whitespace
  assert(parse_ok("2020 0229", "%Y%n%m%d", ymd) && ymd == year{2020} / February / 29);
  assert(parse_ok("2020 0229", "%Y%t%m%d", ymd) && ymd == year{2020} / February / 29);
  assert(parse_ok("2020 0229", "%Y%%", ymd) == false);
  assert(parse_ok("100% 2020-01-01", "100%% %F", ymd) && ymd == year{2020} / January / 1);
  assert(parse_ok("2020-02-29 trailing", "%F", ymd)); // trailing input is left in the stream
  assert(parse_ok("2020/02/29", "%F", ymd) == false);
  assert(parse_ok("2020-02-2", "%F", ymd) && ymd == year{2020} / February / 2);
}

static void test_durations() {
  {
    seconds d{};
    assert(parse_ok("01:02", "%R", d) == true && d == 1h + 2min);
    assert(parse_ok("12:34:56", "%T", d) && d == 12h + 34min + 56s);
    assert(parse_ok("-01:02:03", "%T", d) && d == -(1h + 2min + 3s)); // negative durations carry a leading '-'
    assert(parse_ok("2", "%j", d) && d == 48h); // LWG 3270: %j is a number of days for durations
    assert(parse_ok("1 02", "%j %H", d) && d == 26h);
    assert(parse_ok("12 AM", "%I %p", d) && d == 0s); // LWG 3272: times since midnight
    assert(parse_ok("12 PM", "%I %p", d) && d == 12h);
    assert(parse_ok("11:30 PM", "%I:%M %p", d) && d == 23h + 30min);
    assert(parse_ok("2020", "%Y", d) == false); // calendar fields are an error for durations
    assert(parse_ok("Mon", "%a", d) == false);
  }
  {
    milliseconds d{};
    assert(parse_ok("00:00:01.250", "%T", d) && d == 1250ms);
    assert(parse_ok("59.999", "%S", d) && d == 59999ms);
    duration<long double> f{};
    assert(parse_ok("00:00:01.250", "%T", f));
    assert(f.count() > 1.249L && f.count() < 1.251L);
  }
  {
    minutes d{5};
    assert(parse_ok("01:02", "%R", d) && d == 62min);
    std::istringstream is("00:00:30");
    from_stream(is, "%T", d); // 30 seconds are not representable in minutes
    assert(is.fail() && d == 62min);
    hours h{};
    assert(parse_ok("03", "%H", h) && h == 3h);
    assert(parse_ok("03:30", "%R", h) == false);
  }
}

static void test_time_points() {
  {
    sys_time<seconds> tp{};
    assert(parse_ok("2021-03-04 05:06:07", "%F %T", tp) && tp == sys_days{year{2021} / March / 4} + 5h + 6min + 7s);
    assert(parse_ok("2021-03-04", "%F", tp) && tp == sys_days{year{2021} / March / 4}); // the time defaults to midnight
    assert(parse_ok("05:06:07", "%T", tp) == false);                                     // a date is required
    assert(parse_ok("2021-03-04 24:00:00", "%F %T", tp) == false);
    assert(parse_ok("2021-03-04 23:59:60", "%F %T", tp) == false); // no leap seconds in a sys_time
    assert(parse_ok("2021-03-04 11:15:16 PM", "%F %I:%M:%S %p", tp) &&
           tp == sys_days{year{2021} / March / 4} + 23h + 15min + 16s);
    assert(parse_ok("2021-03-04 12:00:00 AM", "%F %I:%M:%S %p", tp) && tp == sys_days{year{2021} / March / 4});
    assert(parse_ok("1970-01-01", "%F", tp) && tp == sys_seconds{});
    assert(parse_ok("1969-12-31 23:59:59", "%F %T", tp) && tp == sys_seconds{-1s});
    assert(parse_ok("Thu Mar  4 05:06:07 2021", "%c", tp) && tp == sys_days{year{2021} / March / 4} + 5h + 6min + 7s);
    assert(parse_ok("03/04/21 05:06:07", "%x %X", tp) && tp == sys_days{year{2021} / March / 4} + 5h + 6min + 7s);
  }
  {
    // %z: the parsed offset is subtracted, and reported through the offset argument.
    sys_time<seconds> tp{};
    minutes offset{};
    std::istringstream is("2021-03-04 05:06:07 +0130");
    from_stream(is, "%F %T %z", tp, static_cast<std::string*>(nullptr), &offset);
    assert(!is.fail());
    assert(offset == 90min);
    assert(tp == sys_days{year{2021} / March / 4} + 3h + 36min + 7s);
    std::istringstream neg("2021-03-04 05:06:07 -08");
    from_stream(neg, "%F %T %z", tp, static_cast<std::string*>(nullptr), &offset);
    assert(!neg.fail() && offset == -8h);
    assert(tp == sys_days{year{2021} / March / 4} + 13h + 6min + 7s);
    std::istringstream colon("2021-03-04 05:06:07 +5:30");
    from_stream(colon, "%F %T %Ez", tp, static_cast<std::string*>(nullptr), &offset);
    assert(!colon.fail() && offset == 5h + 30min);
    std::istringstream badoffset("2021-03-04 05:06:07 +2460");
    from_stream(badoffset, "%F %T %z", tp);
    assert(badoffset.fail());
  }
  {
    sys_time<milliseconds> tp{};
    assert(parse_ok("2021-03-04 05:06:07.123", "%F %T", tp) &&
           tp == sys_days{year{2021} / March / 4} + 5h + 6min + 7s + 123ms);
    // The fractional digits are optional: "up to" width characters are read.
    assert(parse_ok("2021-03-04 05:06:07", "%F %T", tp) && tp == sys_days{year{2021} / March / 4} + 5h + 6min + 7s);
    sys_time<hours> h{};
    assert(parse_ok("2021-03-04 05", "%F %H", h) && h == sys_days{year{2021} / March / 4} + 5h);
    assert(parse_ok("2021-03-04 05:06", "%F %R", h) == false);
    sys_days d{};
    assert(parse_ok("2021-03-04", "%F", d) && d == year{2021} / March / 4);
  }
  {
    local_time<seconds> tp{};
    minutes offset{};
    std::istringstream is("2021-03-04 05:06:07 +0130");
    from_stream(is, "%F %T %z", tp, static_cast<std::string*>(nullptr), &offset);
    assert(!is.fail() && offset == 90min);
    assert(tp == local_days{year{2021} / March / 4} + 5h + 6min + 7s); // a local_time is not adjusted
  }
  {
    std::string abbrev;
    sys_time<seconds> tp{};
    std::istringstream is("2021-03-04 05:06:07 CEST");
    from_stream(is, "%F %T %Z", tp, &abbrev);
    assert(!is.fail() && abbrev == "CEST");
    std::istringstream slash("2021-03-04 05:06:07 America/New_York");
    from_stream(slash, "%F %T %Z", tp, &abbrev);
    assert(!slash.fail() && abbrev == "America/New_York");
    std::istringstream plus("2021-03-04 05:06:07 Etc/GMT+5 rest");
    from_stream(plus, "%F %T %Z", tp, &abbrev);
    assert(!plus.fail() && abbrev == "Etc/GMT+5");
  }
  {
    file_time<seconds> tp{};
    assert(parse_ok("2021-03-04 05:06:07", "%F %T", tp));
    assert(tp == file_clock::from_sys(sys_days{year{2021} / March / 4} + 5h + 6min + 7s));
  }
}

static void test_stream_state() {
  {
    std::istringstream is("2020-02-29");
    year_month_day ymd{};
    from_stream(is, "%F", ymd);
    assert(!is.fail());
    assert(is.peek() == std::char_traits<char>::eof()); // the input was consumed up to the end
  }
  {
    std::istringstream is("2020-02-29 more");
    year_month_day ymd{};
    from_stream(is, "%F", ymd);
    assert(is.good());
    std::string rest;
    std::getline(is, rest);
    assert(rest == " more");
  }
  {
    std::istringstream is("abc");
    year_month_day ymd{};
    from_stream(is, "%F", ymd);
    assert(is.fail());
  }
  {
    // A stream that is already in a failed state is left alone.
    std::istringstream is("2020-02-29");
    is.setstate(std::ios_base::failbit);
    year_month_day ymd{2000y / January / 1};
    from_stream(is, "%F", ymd);
    assert(is.fail() && ymd == 2000y / January / 1);
  }
  {
    // Leading whitespace is not skipped by from_stream itself.
    std::istringstream is(" 2020-02-29");
    year_month_day ymd{};
    from_stream(is, "%F", ymd);
    assert(is.fail());
  }
}

#if !defined(TEST_HAS_NO_WIDE_CHARACTERS)
static void test_wide() {
  std::wistringstream is(L"2021-03-04 05:06:07 CEST");
  sys_time<seconds> tp{};
  std::wstring abbrev;
  from_stream(is, L"%F %T %Z", tp, &abbrev);
  assert(!is.fail() && abbrev == L"CEST");
  assert(tp == sys_days{year{2021} / March / 4} + 5h + 6min + 7s);
  std::wistringstream m(L"SEPTEMBER");
  month mo{};
  from_stream(m, L"%b", mo);
  assert(!m.fail() && mo == September); // full names are matched case-insensitively
}
#endif

int main(int, char**) {
  test_day_month_year_weekday();
  test_composites();
  test_literals_and_whitespace();
  test_durations();
  test_time_points();
  test_stream_state();
#if !defined(TEST_HAS_NO_WIDE_CHARACTERS)
  test_wide();
#endif
  return 0;
}
