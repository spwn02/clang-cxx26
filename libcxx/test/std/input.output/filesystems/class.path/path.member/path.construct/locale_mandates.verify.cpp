//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14
// REQUIRES: localization

#include <filesystem>
#include <locale>
#include <string>

void test() {
  const std::locale& loc = std::locale::classic();
  std::wstring source    = L"bad";
  std::filesystem::path from_source(source, loc); // expected-error@* {{path source with a locale must have char value type}}
  std::filesystem::path from_range(source.begin(), source.end(), loc); // expected-error@* {{path input iterator with a locale must have char value type}}
}
