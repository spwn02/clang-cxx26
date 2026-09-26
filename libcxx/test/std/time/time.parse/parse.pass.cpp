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

// [time.parse]: the parse manipulators.
//
// template<class charT, class Parsable>
//   unspecified parse(const charT* fmt, Parsable& tp);                         // LWG 3554
// template<class charT, class traits, class Alloc, class Parsable>
//   unspecified parse(const basic_string<charT, traits, Alloc>& fmt, Parsable& tp);
// ... the same with `abbrev`, `offset`, and both.

#include <cassert>
#include <chrono>
#include <concepts>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>

#include "test_macros.h"

using namespace std::chrono;
using namespace std::literals::chrono_literals;

// A user type with its own from_stream, found by ADL.
namespace user {
struct Token {
  std::string text;
};
template <class CharT, class Traits>
std::basic_istream<CharT, Traits>& from_stream(std::basic_istream<CharT, Traits>& is, const CharT* fmt, Token& t) {
  is >> t.text;
  (void)fmt;
  return is;
}
} // namespace user

template <class... Args>
concept can_parse = requires(Args&&... args) { std::chrono::parse(std::forward<Args>(args)...); };

static void test_forms() {
  // 1. (fmt, tp): const charT* and basic_string
  {
    year_month_day ymd{};
    std::istringstream is("2020-02-29");
    is >> parse("%F", ymd);
    assert(!is.fail() && ymd == year{2020} / February / 29);

    std::string fmt = "%Y/%m/%d";
    std::istringstream is2("2021/03/04");
    is2 >> parse(fmt, ymd);
    assert(!is2.fail() && ymd == year{2021} / March / 4);
  }
  // 2. (fmt, tp, abbrev)
  {
    sys_time<seconds> tp{};
    std::string abbrev;
    std::istringstream is("2021-03-04 05:06:07 CET");
    is >> parse("%F %T %Z", tp, abbrev);
    assert(!is.fail() && abbrev == "CET");

    std::string fmt = "%F %T %Z";
    abbrev.clear();
    std::istringstream is2("2021-03-04 05:06:07 EST");
    is2 >> parse(fmt, tp, abbrev);
    assert(!is2.fail() && abbrev == "EST");
  }
  // 3. (fmt, tp, offset)
  {
    sys_time<seconds> tp{};
    minutes offset{};
    std::istringstream is("2021-03-04 05:06:07 +0100");
    is >> parse("%F %T %z", tp, offset);
    assert(!is.fail() && offset == 60min);
    assert(tp == sys_days{year{2021} / March / 4} + 4h + 6min + 7s);

    std::string fmt = "%F %T %z";
    std::istringstream is2("2021-03-04 05:06:07 -0130");
    is2 >> parse(fmt, tp, offset);
    assert(!is2.fail() && offset == -90min);
  }
  // 4. (fmt, tp, abbrev, offset)
  {
    sys_time<seconds> tp{};
    std::string abbrev;
    minutes offset{};
    std::istringstream is("2021-03-04 05:06:07 +0100 CET");
    is >> parse("%F %T %z %Z", tp, abbrev, offset);
    assert(!is.fail() && abbrev == "CET" && offset == 60min);

    std::string fmt = "%F %T %z %Z";
    abbrev.clear();
    std::istringstream is2("2021-03-04 05:06:07 +0200 EET");
    is2 >> parse(fmt, tp, abbrev, offset);
    assert(!is2.fail() && abbrev == "EET" && offset == 120min);
  }
  // Extracting more than one value from the same stream, and the stream is returned (LWG 3269).
  {
    std::istringstream is("2020-02-29 12:34");
    year_month_day ymd{};
    minutes m{};
    std::istream& ret = (is >> parse("%F ", ymd) >> parse("%H:%M", m));
    assert(&ret == &is);
    assert(!is.fail() && ymd == year{2020} / February / 29 && m == 12h + 34min);
  }
  // A failed parse sets failbit and returns the stream.
  {
    std::istringstream is("nonsense");
    year_month_day ymd{};
    std::istream& ret = (is >> parse("%F", ymd));
    assert(&ret == &is && is.fail());
  }
  // LWG 3235: the manipulator works without an abbreviation argument for a Parsable whose
  // from_stream has no abbreviation/offset parameters.
  {
    user::Token t;
    std::istringstream is("hello");
    is >> parse("%x", t);
    assert(t.text == "hello");
  }
}

static void test_constraints() {
  year_month_day ymd{};
  std::string abbrev;
  minutes offset{};
  static_assert(can_parse<const char (&)[3], year_month_day&>);
  static_assert(can_parse<const char*, year_month_day&>);
  static_assert(can_parse<const std::string&, year_month_day&>);
  static_assert(can_parse<const char*, year_month_day&, std::string&>);
  static_assert(can_parse<const char*, year_month_day&, minutes&>);
  static_assert(can_parse<const char*, year_month_day&, std::string&, minutes&>);
  static_assert(can_parse<const std::string&, year_month_day&, std::string&, minutes&>);
  // No from_stream for these types.
  static_assert(!can_parse<const char*, int&>);
  static_assert(!can_parse<const char*, hh_mm_ss<seconds>&>);
  static_assert(!can_parse<const char*, year_month_day_last&>);
  static_assert(!can_parse<const char*, weekday_indexed&>);
  // The abbreviation must be a basic_string over the same character type.
#if !defined(TEST_HAS_NO_WIDE_CHARACTERS)
  static_assert(!can_parse<const char*, year_month_day&, std::wstring&>);
#endif
  (void)ymd;
  (void)abbrev;
  (void)offset;

  // The manipulator is not meant to be stored or copied.
  auto manip = [&] { return parse("%F", ymd); };
  using M = decltype(manip());
  static_assert(!std::is_copy_constructible_v<M>);
  static_assert(!std::is_move_constructible_v<M>);
  static_assert(!std::is_copy_assignable_v<M>);
}

#if !defined(TEST_HAS_NO_WIDE_CHARACTERS)
static void test_wide() {
  sys_time<seconds> tp{};
  std::wstring abbrev;
  minutes offset{};
  std::wistringstream is(L"2021-03-04 05:06:07 +0100 CET");
  is >> parse(L"%F %T %z %Z", tp, abbrev, offset);
  assert(!is.fail() && abbrev == L"CET" && offset == 60min);
  std::wstring fmt = L"%F";
  year_month_day ymd{};
  std::wistringstream is2(L"2020-02-29");
  is2 >> parse(fmt, ymd);
  assert(!is2.fail() && ymd == year{2020} / February / 29);
}
#endif

int main(int, char**) {
  test_forms();
  test_constraints();
#if !defined(TEST_HAS_NO_WIDE_CHARACTERS)
  test_wide();
#endif
  return 0;
}
