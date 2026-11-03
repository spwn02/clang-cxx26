//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23, c++26
// UNSUPPORTED: no-localization
// UNSUPPORTED: no-wide-characters

// <system_error>

// P3395R6: the message is in the execution character set, which for a program that never calls setlocale is
// the one of the "C" locale. Bytes that are not valid characters of that locale are replaced by U+FFFD, so a
// UTF-8 message is only passed through unchanged after the program selects a UTF-8 locale.

#include <cassert>
#include <cwchar>
#include <format>
#include <string>
#include <string_view>
#include <system_error>

struct utf8_category : std::error_category {
  const char* name() const noexcept override { return "utf8"; }
  std::string message(int) const override { return "caf\xC3\xA9"; } // UTF-8
};

int main(int, char**) {
  utf8_category cat;
  std::error_code ec(1, cat);
  std::string result = std::vformat(std::string_view("{:s}"), std::make_format_args(ec));

  // The ASCII part is always unchanged.
  assert(result.compare(0, 3, "caf") == 0);

  // Does the "C" locale of this platform accept the byte 0xC3 as a character?
  std::mbstate_t state = std::mbstate_t();
  wchar_t wc           = 0;
  bool c_locale_rejects_0xc3 = std::mbrtowc(&wc, "\xC3", 1, &state) == static_cast<std::size_t>(-1);
  if (c_locale_rejects_0xc3) // e.g. glibc: the "C" locale is ASCII
    assert(result == "caf\xEF\xBF\xBD\xEF\xBF\xBD");

  return 0;
}
