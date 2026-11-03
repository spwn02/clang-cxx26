//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23, c++26
// UNSUPPORTED: no-localization
// REQUIRES: locale.fr_CA.ISO8859-1

// <system_error>

// P3395R6: the message is in the execution character set and is transcoded to UTF-8.
// Under an ISO 8859-1 locale a byte 0xE9 is "e acute" and becomes the two bytes C3 A9.

#include <cassert>
#include <clocale>
#include <format>
#include <string>
#include <string_view>
#include <system_error>

#include "platform_support.h"
#include "test_macros.h"

struct latin1_category : std::error_category {
  const char* name() const noexcept override { return "latin1"; }
  std::string message(int) const override { return "caf\xE9"; } // ISO 8859-1
};

int main(int, char**) {
  assert(std::setlocale(LC_ALL, LOCALE_fr_CA_ISO8859_1) != nullptr);
  latin1_category cat;
  std::error_code ec(1, cat);
  assert(std::vformat(std::string_view("{:s}"), std::make_format_args(ec)) == "caf\xC3\xA9");
#ifndef TEST_HAS_NO_WIDE_CHARACTERS
  assert(std::vformat(std::wstring_view(L"{:s}"), std::make_wformat_args(ec)) == L"caf\u00e9");
#endif
  return 0;
}
