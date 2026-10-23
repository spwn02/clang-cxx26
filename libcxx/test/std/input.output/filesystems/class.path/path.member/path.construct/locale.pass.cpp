//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14
// REQUIRES: localization

#include <cassert>
#include <filesystem>
#include <locale>
#include <string>

int main(int, char**) {
  const std::locale& loc = std::locale::classic();
  std::string source     = "hello";
  std::filesystem::path from_source(source, loc);
  std::filesystem::path from_range(source.begin(), source.end(), loc);
  assert(from_source == std::filesystem::path("hello"));
  assert(from_range == from_source);
  return 0;
}
