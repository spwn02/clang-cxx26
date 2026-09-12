//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17
// UNSUPPORTED: no-localization
// REQUIRES: locale.fr_CA.ISO8859-1

// <chrono>
// P2419R2: Clarify handling of encodings in localized formatting of chrono types

#include <chrono>
#include <format>
#include <locale>

#include <cassert>

#include "platform_support.h"

int main(int, char**) {
  using namespace std::chrono;

  const std::locale loc(LOCALE_fr_CA_ISO8859_1);
  const sys_days date = 2024y / August / 1;

  // The locale produces "août" in ISO-8859-1. The UTF-8 source literal
  // encoding requires the localized replacement to be converted to UTF-8.
  assert(std::format(loc, "{:L%B}", date) == "août");
}
