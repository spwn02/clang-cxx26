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

#if TEST_STD_VER < 26
constexpr bool test() {
  std::format_parse_context context("", 10);
  for (std::size_t i = 0; i < 10; ++i)
    context.check_arg_id(i);

  return true;
}
#else
// From C++26 the public constructor has no number of arguments (num_args_ == 0): the call works at run time but is
// never a constant expression (see check_arg_id.verify.cpp).
bool test() {
  std::format_parse_context context("");
  for (std::size_t i = 0; i < 10; ++i)
    context.check_arg_id(i);

  return true;
}
#endif

void test_exception() {
  [] {
#if TEST_STD_VER < 26
    std::format_parse_context context("", 1);
#else
    std::format_parse_context context("");
#endif
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

#if TEST_STD_VER < 26
  auto test_arg = [](std::size_t num_args) {
    std::format_parse_context context("", num_args);
    // Out of bounds access is valid if !std::is_constant_evaluated()
    for (std::size_t i = 0; i <= num_args; ++i)
      context.check_arg_id(i);
  };
  for (std::size_t i = 0; i < 10; ++i)
    test_arg(i);
#else
  // Any id is accepted at run time.
  std::format_parse_context context("");
  for (std::size_t i = 0; i <= 10; ++i)
    context.check_arg_id(i);
#endif
}

int main(int, char**) {
  test();
  test_exception();
#if TEST_STD_VER < 26
  static_assert(test());
#endif

  return 0;
}
