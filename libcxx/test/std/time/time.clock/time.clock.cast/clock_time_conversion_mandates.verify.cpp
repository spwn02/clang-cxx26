// REQUIRES: std-at-least-c++20
// UNSUPPORTED: no-filesystem, no-localization, no-tzdb

// <chrono>
// Mandates the clock_time_conversion result to match its clock.

#include <chrono>

// expected-note@*:* 0+ {{in instantiation of}}

struct bad_source_clock {
  using duration   = std::chrono::seconds;
  using time_point = std::chrono::time_point<bad_source_clock>;

  static int to_sys(time_point);
};

struct bad_destination_clock {
  static int from_sys(std::chrono::sys_seconds);
};

struct bad_utc_source_clock {
  using duration   = std::chrono::seconds;
  using time_point = std::chrono::time_point<bad_utc_source_clock>;

  static int to_utc(time_point);
};

struct bad_utc_destination_clock {
  static int from_utc(std::chrono::utc_seconds);
};

void source_result_mandates() {
  (void)std::chrono::clock_time_conversion<std::chrono::system_clock, bad_source_clock>{}(
      bad_source_clock::time_point{}); // expected-error@*:* {{static assertion failed}}
}

void destination_result_mandates() {
  (void)std::chrono::clock_time_conversion<bad_destination_clock, std::chrono::system_clock>{}(
      std::chrono::sys_seconds{}); // expected-error@*:* {{static assertion failed}}
}

void utc_source_result_mandates() {
  (void)std::chrono::clock_time_conversion<std::chrono::utc_clock, bad_utc_source_clock>{}(
      bad_utc_source_clock::time_point{}); // expected-error@*:* {{static assertion failed}}
}

void utc_destination_result_mandates() {
  (void)std::chrono::clock_time_conversion<bad_utc_destination_clock, std::chrono::utc_clock>{}(
      std::chrono::utc_seconds{}); // expected-error@*:* {{static assertion failed}}
}
