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

#include <stacktrace>

#include "check_assertion.h"

int main(int, char**) {
  std::stacktrace trace;
  TEST_LIBCPP_ASSERT_FAILURE(trace[0], "basic_stacktrace::operator[] index out of bounds");

  const std::stacktrace const_trace;
  TEST_LIBCPP_ASSERT_FAILURE(const_trace[0], "basic_stacktrace::operator[] index out of bounds");

  return 0;
}
