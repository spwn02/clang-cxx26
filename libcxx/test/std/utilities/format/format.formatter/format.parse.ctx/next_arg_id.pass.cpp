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

// constexpr size_t next_arg_id();

#include <format>

#include <cassert>
#include <cstring>
#include <string_view>

#include "test_macros.h"

// A context built by the public constructor has num_args_ == 0: the calls work at run time but are never constant
// expressions (see next_arg_id.verify.cpp).
bool test() {
  std::format_parse_context context("");
  for (std::size_t i = 0; i < 10; ++i)
    assert(i == context.next_arg_id());

  return true;
}

void test_exception() {
  std::format_parse_context context("");
  context.check_arg_id(0);

  try {
    TEST_IGNORE_NODISCARD context.next_arg_id();
    assert(false);
  } catch ([[maybe_unused]] const std::format_error& e) {
    LIBCPP_ASSERT(std::strcmp(e.what(), "Using automatic argument numbering in manual argument numbering mode") == 0);
    return;
  }
  assert(false);
}

int main(int, char**) {
  test();
  test_exception();

  return 0;
}
