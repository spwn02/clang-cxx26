//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <format>

// P3378R2 (constexpr exception types), [format.error]:
//   constexpr explicit format_error(const string& what_arg);
//   constexpr explicit format_error(const char* what_arg);

#include <cassert>
#include <format>
#include <string>
#include <string_view>

constexpr bool test() {
  {
    std::format_error e("abc");
    if (std::string_view(e.what()) != "abc")
      return false;
    std::format_error copy(e);
    if (std::string_view(copy.what()) != "abc")
      return false;
    std::format_error other("other");
    other = e;
    if (std::string_view(other.what()) != "abc")
      return false;
  }
  {
    std::string message("a longer message that does not fit a small string buffer");
    std::format_error e(message);
    if (std::string_view(e.what()) != message)
      return false;
  }
  return true;
}

int main(int, char**) {
  assert(test());
  static_assert(test());
  return 0;
}
