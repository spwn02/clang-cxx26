//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// REQUIRES: std-at-least-c++20
// UNSUPPORTED: no-filesystem, no-localization, no-tzdb

// XFAIL: libcpp-has-no-experimental-tzdb
// XFAIL: availability-tzdb-missing

// <chrono>
//
// template<class DestClock, class SourceClock>
//   struct clock_time_conversion;
//
// template<class DestClock, class SourceClock, class Duration>
//   auto clock_cast(const time_point<SourceClock, Duration>& t);

#include <chrono>
#include <cassert>
#include <type_traits>

struct identity_clock {
  using rep                       = long long;
  using period                    = std::ratio<1>;
  using duration                  = std::chrono::seconds;
  using time_point                = std::chrono::time_point<identity_clock>;
  static constexpr bool is_steady = false;
};

struct source_clock : identity_clock {
  using time_point = std::chrono::time_point<source_clock>;
};

struct destination_clock : identity_clock {
  using time_point = std::chrono::time_point<destination_clock>;
};

template <>
struct std::chrono::clock_time_conversion<destination_clock, source_clock> {
  template <class _Duration>
  std::chrono::time_point<destination_clock, _Duration>
  operator()(const std::chrono::time_point<source_clock, _Duration>& t) const {
    return std::chrono::time_point<destination_clock, _Duration>{t.time_since_epoch() + _Duration{1}};
  }
};

template <class _DestClock, class _SourceClock, class _Duration>
concept HasClockCast = requires(const std::chrono::time_point<_SourceClock, _Duration>& t) {
  std::chrono::clock_cast<_DestClock>(t);
};

static_assert(std::is_empty_v<std::chrono::clock_time_conversion<identity_clock, std::chrono::steady_clock>>);
static_assert(!HasClockCast<std::chrono::system_clock, std::chrono::steady_clock, std::chrono::seconds>);

int main(int, char**) {
  namespace cr = std::chrono;
  using namespace std::chrono_literals;

  // Identity conversions preserve both clock and duration.
  {
    const cr::time_point<identity_clock, cr::milliseconds> input{1234ms};
    static_assert(std::is_same_v<decltype(cr::clock_cast<identity_clock>(input)),
                                 cr::time_point<identity_clock, cr::milliseconds>>);
    assert(cr::clock_cast<identity_clock>(input) == input);
  }

  // User specializations provide direct conversions.
  {
    const cr::time_point<source_clock, cr::seconds> input{3s};
    assert(cr::clock_cast<destination_clock>(input).time_since_epoch() == 4s);
  }

  const cr::sys_seconds sys = cr::sys_days{cr::January / 1 / 2020} + 12h + 34min + 56s;

  // Direct conversions between system_clock and clocks with to_sys/from_sys.
  {
    const auto file = cr::clock_cast<cr::file_clock>(sys);
    static_assert(std::is_same_v<std::remove_const_t<decltype(file)>, cr::file_time<cr::seconds>>);
    assert(file == cr::file_clock::from_sys(sys));
    assert(cr::clock_cast<cr::system_clock>(file) == sys);
  }
  {
    const auto utc = cr::clock_cast<cr::utc_clock>(sys);
    static_assert(std::is_same_v<std::remove_const_t<decltype(utc)>, cr::utc_time<cr::seconds>>);
    assert(utc == cr::utc_clock::from_sys(sys));
    assert(cr::clock_cast<cr::system_clock>(utc) == sys);
  }

  // tai_clock and gps_clock convert to system_clock through utc_clock.
  {
    const auto tai = cr::clock_cast<cr::tai_clock>(sys);
    assert(tai == cr::tai_clock::from_utc(cr::utc_clock::from_sys(sys)));
    assert(cr::clock_cast<cr::system_clock>(tai) == sys);
  }
  {
    const auto gps = cr::clock_cast<cr::gps_clock>(sys);
    assert(gps == cr::gps_clock::from_utc(cr::utc_clock::from_sys(sys)));
    assert(cr::clock_cast<cr::system_clock>(gps) == sys);
  }

  // No direct conversion exists in either case; all three conversion steps
  // selected by clock_cast are required.
  {
    const cr::file_time<cr::seconds> file = cr::file_clock::from_sys(sys);
    assert(cr::clock_cast<cr::tai_clock>(file) ==
           cr::tai_clock::from_utc(cr::utc_clock::from_sys(cr::file_clock::to_sys(file))));
  }
  {
    const cr::gps_time<cr::seconds> gps = cr::gps_clock::from_utc(cr::utc_clock::from_sys(sys));
    assert(cr::clock_cast<cr::file_clock>(gps) ==
           cr::file_clock::from_sys(cr::utc_clock::to_sys(cr::gps_clock::to_utc(gps))));
  }

  return 0;
}
