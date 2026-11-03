//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23, c++26
// UNSUPPORTED: no-localization
// REQUIRES: locale.en_US.UTF-8

// <system_error>

// P3395R6: template<class charT> struct formatter<error_code, charT>;
//
//   error-code-format-spec: fill-and-align(opt) width(opt) ?(opt) s(opt)

#include <cassert>
#include <concepts>
#include <string_view>
#include <clocale>
#include <format>
#include <map>
#include <string>
#include <system_error>
#include <utility>

#include "assert_macros.h"
#include "concat_macros.h"
#include "make_string.h"
#include "platform_support.h"
#include "test_macros.h"

// A category whose message is whatever the test says.
struct test_category : std::error_category {
  const char* name() const noexcept override { return "test"; }
  std::string message(int ev) const override {
    if (ev == 1)
      return "plain message";
    if (ev == 2) // Unicode 3.9, Table 3-8: 61 F1 80 80 E1 80 C2 62 80 63 80 BF 64
      return "a\xF1\x80\x80\xE1\x80\xC2"
             "b\x80"
             "c\x80\xBF"
             "d";
    if (ev == 3)
      return "caf\xC3\xA9"; // well-formed UTF-8
    if (ev == 4)
      return "quote\" tab\t";
    if (ev == 5)
      return "caf\xE9"; // ISO 8859-1
    return "unknown";
  }
};
const test_category& cat() {
  static test_category c;
  return c;
}

template <class CharT>
using S = std::basic_string<CharT>;

// The format strings below are not constant expressions (they are built from the macro), so go through vformat.
template <class CharT, class... Args>
S<CharT> fmt(std::basic_string_view<CharT> f, Args... args) {
  if constexpr (std::same_as<CharT, char>)
    return std::vformat(f, std::make_format_args(args...));
#ifndef TEST_HAS_NO_WIDE_CHARACTERS
  else
    return std::vformat(f, std::make_wformat_args(args...));
#endif
}

#define SV(s) MAKE_STRING(CharT, s)

template <class CharT>
void test_default_and_width() {
  std::error_code ec(7, cat());
  assert(fmt<CharT>(SV("{}"), ec) == SV("test:7"));
  assert(fmt<CharT>(SV("[{:>12}]"), ec) == SV("[      test:7]"));
  assert(fmt<CharT>(SV("[{:*^12}]"), ec) == SV("[***test:7***]"));
  assert(fmt<CharT>(SV("[{:<9}]"), ec) == SV("[test:7   ]"));
  assert(fmt<CharT>(SV("[{:12}]"), ec) == SV("[test:7      ]"));
  assert(fmt<CharT>(SV("{}"), std::error_code()) == SV("system:0"));
  assert(fmt<CharT>(SV("{}"), std::make_error_code(std::errc::no_such_file_or_directory)) == SV("generic:2"));
}

template <class CharT>
void test_message() {
  assert(fmt<CharT>(SV("{:s}"), std::error_code(1, cat())) == SV("plain message"));
  assert(fmt<CharT>(SV("[{:>15s}]"), std::error_code(1, cat())) == SV("[  plain message]"));
  // the generic category's message is platform dependent, but not empty
  assert(!fmt<CharT>(SV("{:s}"), std::make_error_code(std::errc::no_such_file_or_directory)).empty());
}

template <class CharT>
void test_debug() {
  assert(fmt<CharT>(SV("{:?}"), std::error_code(7, cat())) == SV("\"test:7\""));
  assert(fmt<CharT>(SV("{:?s}"), std::error_code(4, cat())) == SV("\"quote\\\" tab\\t\""));
  assert(fmt<CharT>(SV("[{:>10?}]"), std::error_code(7, cat())) == SV("[  \"test:7\"]"));
  // set_debug_format() is used by the range formatter: the ':' cannot be mistaken for the key/value separator
  std::map<std::error_code, int> m{{std::error_code(7, cat()), 1}};
  assert(fmt<CharT>(SV("{}"), m) == SV("{\"test:7\": 1}"));
  std::formatter<std::error_code, CharT> f;
  f.set_debug_format();
}

template <class CharT>
void test_invalid_specs() {
  std::error_code ec(7, cat());
  auto format_runtime = [&](std::basic_string_view<CharT> fmt) {
    if constexpr (std::same_as<CharT, char>)
      return std::vformat(fmt, std::make_format_args(ec));
#ifndef TEST_HAS_NO_WIDE_CHARACTERS
    else
      return std::vformat(fmt, std::make_wformat_args(ec));
#endif
  };
  TEST_THROWS_TYPE(std::format_error, (void)format_runtime(SV("{:s?}")));
  TEST_THROWS_TYPE(std::format_error, (void)format_runtime(SV("{:x}")));
  TEST_THROWS_TYPE(std::format_error, (void)format_runtime(SV("{:ss}")));
}

// A category whose message is the string given at construction.
struct fixed_message_category : std::error_category {
  explicit fixed_message_category(std::string m) : msg(std::move(m)) {}
  const char* name() const noexcept override { return "fixed"; }
  std::string message(int) const override { return msg; }
  std::string msg;
};

std::string format_message(std::string message) {
  fixed_message_category c(std::move(message));
  std::error_code ec(1, c);
  return std::vformat(std::string_view("{:s}"), std::make_format_args(ec));
}

// The vectors of Tables 3-8 to 3-11 of the Unicode Standard, Chapter 3.9 (U+FFFD Substitution of Maximal Subparts).
void test_unicode_tables() {
  const std::string R = "\xEF\xBF\xBD"; // U+FFFD
  // Table 3-8: non-shortest form sequences, every byte is replaced
  assert(format_message("\xC0\xAF\xE0\x80\xBF\xF0\x81\x82\x41") == R + R + R + R + R + R + R + R + "A");
  // Table 3-9: surrogates
  assert(format_message("\xED\xA0\x80\xED\xBF\xBF\xED\xAF\x41") == R + R + R + R + R + R + R + R + "A");
  // Table 3-10: other ill-formed sequences
  assert(format_message("\xF4\x91\x92\x93\xFF\x41\x80\xBF\x42") == R + R + R + R + R + "A" + R + R + "B");
  // Table 3-11: truncated sequences, one U+FFFD per maximal subpart
  assert(format_message("\xE1\x80\xE2\xF0\x91\x92\xF1\xBF\x41") == R + R + R + R + "A");
  // a truncated sequence at the end of the message
  assert(format_message("ab\xE1\x80") == "ab" + R);
}

void test_utf8_transcoding() {
  assert(std::setlocale(LC_ALL, LOCALE_en_US_UTF_8) != nullptr);
  test_unicode_tables();
  // ill-formed subsequences are replaced one maximal subpart at a time
  assert(fmt<char>(std::string_view("{:s}"), std::error_code(2, cat())) ==
         "a\xEF\xBF\xBD\xEF\xBF\xBD\xEF\xBF\xBD"
         "b\xEF\xBF\xBD"
         "c\xEF\xBF\xBD\xEF\xBF\xBD"
         "d");
  // well-formed UTF-8 is unchanged
  assert(fmt<char>(std::string_view("{:s}"), std::error_code(3, cat())) == "caf\xC3\xA9");
  // the width is measured in the transcoded text: "café" has width 4
  assert(fmt<char>(std::string_view("[{:6s}]"), std::error_code(3, cat())) == "[caf\xC3\xA9  ]");
#ifndef TEST_HAS_NO_WIDE_CHARACTERS
  assert(fmt<wchar_t>(std::wstring_view(L"{:s}"), std::error_code(3, cat())) == L"café");
#endif
}

template <class CharT>
void test() {
  test_default_and_width<CharT>();
  test_message<CharT>();
  test_debug<CharT>();
  test_invalid_specs<CharT>();
}

int main(int, char**) {
  test<char>();
#ifndef TEST_HAS_NO_WIDE_CHARACTERS
  test<wchar_t>();
#endif
  test_utf8_transcoding();
  return 0;
}
