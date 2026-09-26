//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17
// UNSUPPORTED: no-filesystem, no-localization, no-tzdb
// XFAIL: libcpp-has-no-experimental-tzdb
// XFAIL: availability-tzdb-missing

// <chrono>

// [time.parse]: from_stream for utc_time, tai_time and gps_time.

#include <cassert>
#include <chrono>
#include <sstream>

using namespace std::chrono;
using namespace std::literals::chrono_literals;

template <class T>
bool parse_ok(const char* input, const char* fmt, T& value) {
  std::istringstream is(input);
  std::chrono::from_stream(is, fmt, value);
  return !is.fail();
}

int main(int, char**) {
  {
    utc_time<seconds> tp{};
    assert(parse_ok("2017-01-01 00:00:00", "%F %T", tp));
    assert(tp == utc_clock::from_sys(sys_days{year{2017} / January / 1}));

    // 2016-12-31 23:59:60 is a leap second; it is only accepted where it exists.
    assert(parse_ok("2016-12-31 23:59:60", "%F %T", tp));
    assert(get_leap_second_info(tp).is_leap_second);
    assert(tp == utc_clock::from_sys(sys_days{year{2017} / January / 1}) - 1s);
    assert(!parse_ok("2016-12-30 23:59:60", "%F %T", tp));
    assert(!parse_ok("2016-12-31 23:59:61", "%F %T", tp));
  }
  {
    tai_time<seconds> tp{};
    assert(parse_ok("2000-01-01 00:00:00", "%F %T", tp));
    // TAI was 32 s ahead of UTC and counts from 1958-01-01.
    assert(tp.time_since_epoch() == 1325376032s);
  }
  {
    gps_time<seconds> tp{};
    assert(parse_ok("2000-01-01 00:00:00", "%F %T", tp));
    // GPS time counts from 1980-01-06 and was 13 s ahead of UTC.
    assert(tp.time_since_epoch() == 630720013s);
  }
  {
    utc_time<milliseconds> tp{};
    minutes offset{};
    std::istringstream is("2021-03-04 05:06:07.250 +0100");
    from_stream(is, "%F %T %z", tp, static_cast<std::string*>(nullptr), &offset);
    assert(!is.fail() && offset == 60min);
    assert(tp == utc_clock::from_sys(sys_days{year{2021} / March / 4} + 4h + 6min + 7s + 250ms));
  }
  return 0;
}
