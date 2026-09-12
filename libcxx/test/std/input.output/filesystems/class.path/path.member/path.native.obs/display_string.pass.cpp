//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <filesystem>

// class path

// std::string display_string() const;
// std::string system_encoded_string() const;
// std::string generic_display_string() const;
// std::string generic_system_encoded_string() const;

#include <filesystem>
#include <format>
#include <cassert>
#include <string>
#include <type_traits>

#include "test_macros.h"

namespace fs = std::filesystem;

int main(int, char**) {
  const fs::path p("abc/def");

  ASSERT_SAME_TYPE(decltype(p.display_string()), std::string);
  ASSERT_SAME_TYPE(decltype(p.system_encoded_string()), std::string);
  ASSERT_SAME_TYPE(decltype(p.generic_display_string()), std::string);
  ASSERT_SAME_TYPE(decltype(p.generic_system_encoded_string()), std::string);

  // [fs.path.observers]: system_encoded_string() has the contract string()
  // has always had; string() is now specified in terms of it.
  assert(p.system_encoded_string() == p.string());
  assert(p.generic_system_encoded_string() == p.generic_string());

  // display_string() is Returns: format("{}", *this), which for the
  // non-debug, non-generic case is exactly string<char>(); likewise
  // generic_display_string() is format("{:g}", *this) == generic_string<char>().
  assert(p.display_string() == std::format("{}", p));
  assert(p.generic_display_string() == std::format("{:g}", p));
  assert(p.display_string() == p.string());
  assert(p.generic_display_string() == p.generic_string());

  return 0;
}
