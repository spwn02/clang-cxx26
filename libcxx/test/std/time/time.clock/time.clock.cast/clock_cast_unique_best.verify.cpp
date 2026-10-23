// REQUIRES: std-at-least-c++20

// <chrono>
// Mandates that clock_cast has a unique best conversion.

#include <chrono>

// expected-note@*:* 0+ {{in instantiation of}}
// expected-note@*:* 0+ {{expression evaluates to}}

struct source_clock {
  using rep                       = long long;
  using period                    = std::ratio<1>;
  using duration                  = std::chrono::seconds;
  using time_point                = std::chrono::time_point<source_clock>;
  static constexpr bool is_steady = false;

  static std::chrono::sys_seconds to_sys(time_point);
  static std::chrono::utc_seconds to_utc(time_point);
};

struct destination_clock {
  using rep                       = long long;
  using period                    = std::ratio<1>;
  using duration                  = std::chrono::seconds;
  using time_point                = std::chrono::time_point<destination_clock>;
  static constexpr bool is_steady = false;

  static time_point from_sys(std::chrono::sys_seconds);
  static time_point from_utc(std::chrono::utc_seconds);
};

void test() {
  // expected-error@*:* {{clock_cast requires a unique best conversion}}
  (void)std::chrono::clock_cast<destination_clock>(source_clock::time_point{});
}
