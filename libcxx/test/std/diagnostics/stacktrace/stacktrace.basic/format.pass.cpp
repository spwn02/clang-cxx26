//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

// <stacktrace>

// template<class Allocator> struct formatter<basic_stacktrace<Allocator>, char>;
//
// P2693R1: format(st) produces the same text as to_string(st)/operator<<.

#include <cassert>
#include <format>
#include <memory_resource>
#include <stacktrace>
#include <string>

int main(int, char**) {
  // An empty stacktrace still formats.
  {
    std::stacktrace empty;
    std::string formatted = std::format("{}", empty);
    std::string expected  = std::to_string(empty);
    assert(formatted == expected);
    assert(formatted.empty());
  }

  // A real, current stack trace round-trips the same way as to_string()/operator<<.
  {
    auto st                = std::stacktrace::current();
    std::string formatted  = std::format("{}", st);
    std::string expected   = std::to_string(st);
    assert(formatted == expected);
  }

  // Width/fill/align are respected.
  {
    std::stacktrace empty;
    std::string padded = std::format("[{:*>5}]", empty);
    assert(padded == "[*****]");
  }

  // The pmr alias's formatter specialization also works (a distinct Allocator
  // instantiation of the same class template).
  {
    auto pst               = std::pmr::stacktrace::current();
    std::string formatted  = std::format("{}", pst);
    std::string expected   = std::to_string(pst);
    assert(formatted == expected);
  }

  return 0;
}
