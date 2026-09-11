//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

// <stacktrace>

// template<> struct formatter<stacktrace_entry, char>;
//
// P2693R1: format(f) produces the same text as to_string(f)/operator<<.

#include <cassert>
#include <format>
#include <stacktrace>
#include <string>

int main(int, char**) {
  std::stacktrace_entry entry;

  // A default-constructed (empty) entry still formats -- to_string() handles it.
  {
    std::string formatted = std::format("{}", entry);
    std::string expected  = std::to_string(entry);
    assert(formatted == expected);
  }

  // Width/fill/align are respected (formatter supports the plain fill-align-width fields).
  {
    std::string base    = std::to_string(entry);
    std::string padded  = std::format("[{:*>{}}]", entry, base.size() + 10);
    assert(padded.size() == base.size() + 10 + 2);
    assert(padded.front() == '[');
    assert(padded.back() == ']');
    assert(padded.substr(1, 10) == std::string(10, '*'));
    assert(padded.substr(11, base.size()) == base);
  }

  // A real, current stack trace's top frame round-trips the same way.
  {
    auto st = std::stacktrace::current();
    if (!st.empty()) {
      std::string formatted = std::format("{}", st[0]);
      std::string expected  = std::to_string(st[0]);
      assert(formatted == expected);
    }
  }

  return 0;
}
