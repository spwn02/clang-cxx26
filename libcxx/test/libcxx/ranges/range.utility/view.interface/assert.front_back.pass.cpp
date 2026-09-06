//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// REQUIRES: has-unix-headers
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20
// UNSUPPORTED: libcpp-hardening-mode=none
// XFAIL: libcpp-hardening-mode=debug && availability-verbose_abort-missing

// <ranges>

#include <ranges>

#include "check_assertion.h"

int main(int, char**) {
  std::ranges::empty_view<int> view;
  TEST_LIBCPP_ASSERT_FAILURE(view.front(), "Precondition `!empty()` not satisfied. `.front()` called on an empty view.");
  TEST_LIBCPP_ASSERT_FAILURE(view.back(), "Precondition `!empty()` not satisfied. `.back()` called on an empty view.");

  const auto const_view = view;
  TEST_LIBCPP_ASSERT_FAILURE(
      const_view.front(), "Precondition `!empty()` not satisfied. `.front()` called on an empty view.");
  TEST_LIBCPP_ASSERT_FAILURE(
      const_view.back(), "Precondition `!empty()` not satisfied. `.back()` called on an empty view.");

  return 0;
}
