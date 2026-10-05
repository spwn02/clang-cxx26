//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17
// UNSUPPORTED: no-exceptions

// <format>

// constexpr void check_arg_id(size_t id);

#include <format>

#include <cassert>
#include <cstring>
#include <string_view>

#include "test_macros.h"

// A context built by the public constructor has num_args_ == 0: the call works at run time but is never a constant
// expression (see check_arg_id.verify.cpp).
bool test() {
  std::format_parse_context context("");
  for (std::size_t i = 0; i < 10; ++i)
    context.check_arg_id(i);

  return true;
}

void test_exception() {
  [] {
    std::format_parse_context context("");
    TEST_IGNORE_NODISCARD context.next_arg_id();
    try {
      context.check_arg_id(0);
      assert(false);
    } catch ([[maybe_unused]] const std::format_error& e) {
      LIBCPP_ASSERT(std::strcmp(e.what(), "Using manual argument numbering in automatic argument numbering mode") == 0);
      return;
    }
    assert(false);
  }();

  // Any id is accepted at run time.
  std::format_parse_context context("");
  for (std::size_t i = 0; i <= 10; ++i)
    context.check_arg_id(i);
}

int main(int, char**) {
  test();
  test_exception();

  return 0;
}
