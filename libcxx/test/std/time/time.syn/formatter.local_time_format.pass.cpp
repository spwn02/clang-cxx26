//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17
// UNSUPPORTED: no-localization
// UNSUPPORTED: GCC-ALWAYS_INLINE-FIXME

// TODO FMT This test should not require std::to_chars(floating-point)
// XFAIL: availability-fp_to_chars-missing

// <chrono>

// template<class Duration>
//   unspecified local_time_format(local_time<Duration> time, const string* abbrev = nullptr,
//                                 const seconds* offset_sec = nullptr);
//
// template<class Duration, class charT>
//   struct formatter<chrono::local-time-format-t<Duration>, charT>;

#include <chrono>
#include <format>

#include <cassert>
#include <string>

#include "formatter_tests.h"
#include "make_string.h"
#include "test_macros.h"

template <class CharT>
static void test() {
  using namespace std::literals::chrono_literals;

  const std::string abbrev = "CEST";
  const std::chrono::seconds offset{7200};
  const std::chrono::seconds negative_offset{-(3 * 3600 + 1800)};
  const std::string utc = "UTC";

  const std::chrono::local_seconds time = std::chrono::local_days{std::chrono::year(2026) / std::chrono::October / 5} + 6h + 7min + 8s;

  // The chrono-specs default to %F %T %Z
  check(SV("2026-10-05 06:07:08 CEST"), SV("{}"), std::chrono::local_time_format(time, &abbrev));
  check(SV("2026-10-05 06:07:08 CEST"), SV("{}"), std::chrono::local_time_format(time, &abbrev, &offset));
  check(SV("2026-10-05 06:07:08 CEST"), SV("{:}"), std::chrono::local_time_format(time, &abbrev));
  check(SV("2026-10-05 06:07:08 UTC"), SV("{}"), std::chrono::local_time_format(time, &utc));

  // %Z is the abbreviation, %z the offset, also in the modified forms
  check(SV("CEST +0200"), SV("{:%Z %z}"), std::chrono::local_time_format(time, &abbrev, &offset));
  check(SV("+02:00 +02:00"), SV("{:%Ez %Oz}"), std::chrono::local_time_format(time, &abbrev, &offset));
  check(SV("-0330"), SV("{:%z}"), std::chrono::local_time_format(time, nullptr, &negative_offset));
  check(SV("-03:30"), SV("{:%Oz}"), std::chrono::local_time_format(time, nullptr, &negative_offset));

  // The other specifiers are those of a local_time
  check(SV("06:07:08"), SV("{:%T}"), std::chrono::local_time_format(time));
  check(SV("Monday 05 October 2026"), SV("{:%A %d %B %Y}"), std::chrono::local_time_format(time));
  check(SV("2026-10-05"), SV("{:%F}"), std::chrono::local_time_format(time, nullptr, nullptr));

  // Sub-second durations
  const std::chrono::local_time<std::chrono::milliseconds> subsecond{
      std::chrono::local_days{std::chrono::year(2026) / std::chrono::October / 5} + 6h + 123ms};
  check(SV("06:00:00.123 CEST"), SV("{:%T %Z}"), std::chrono::local_time_format(subsecond, &abbrev));
  check(SV("2026-10-05 06:00:00.123 CEST"), SV("{}"), std::chrono::local_time_format(subsecond, &abbrev));

  // The format-spec: fill, align and width apply to the whole result
  check(SV("   CEST|"), SV("{:>7%Z}|"), std::chrono::local_time_format(time, &abbrev));
  check(SV("CEST   |"), SV("{:<7%Z}|"), std::chrono::local_time_format(time, &abbrev));

  // A supplied abbreviation or offset that is not used does not matter; %Z and %z need theirs
  check_exception("The abbreviation of a local_time_format is needed to format %Z", SV("{:%Z}"), std::chrono::local_time_format(time));
  check_exception("The abbreviation of a local_time_format is needed to format %Z", SV("{}"), std::chrono::local_time_format(time));
  check_exception("The abbreviation of a local_time_format is needed to format %Z", SV("{:%Z}"), std::chrono::local_time_format(time, nullptr, &offset));
  check_exception("The offset of a local_time_format is needed to format %z", SV("{:%z}"), std::chrono::local_time_format(time));
  check_exception("The offset of a local_time_format is needed to format %z", SV("{:%z}"), std::chrono::local_time_format(time, &abbrev));
  check_exception("The offset of a local_time_format is needed to format %Ez or %Oz", SV("{:%Ez}"), std::chrono::local_time_format(time, &abbrev));
  check_exception("The offset of a local_time_format is needed to format %Ez or %Oz", SV("{:%Oz}"), std::chrono::local_time_format(time, &abbrev));

  // A local_time itself still has no zone
  check_exception("The supplied date time doesn't contain a time zone", SV("{:%Z}"), time);
  check_exception("The supplied date time doesn't contain a time zone", SV("{:%z}"), time);

  // Invalid format specifications
  check_exception("The format specifier expects a '%' or a '}'", SV("{:A"), std::chrono::local_time_format(time));
  check_exception("End of input while parsing a conversion specifier", SV("{:%"), std::chrono::local_time_format(time));
}

int main(int, char**) {
  test<char>();

#ifndef TEST_HAS_NO_WIDE_CHARACTERS
  test<wchar_t>();
#endif

  static_assert(std::formattable<decltype(std::chrono::local_time_format(std::chrono::local_seconds{})), char>);

  return 0;
}
