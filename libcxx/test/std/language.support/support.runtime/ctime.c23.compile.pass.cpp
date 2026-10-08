//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: windows

// <ctime>

// The functions that C23 adds to <time.h> and that [ctime.syn] declares in C++26:
//   time_t timegm(tm*);
//   int timespec_getres(timespec*, int);
//   tm* gmtime_r(const time_t*, tm*);
//   tm* localtime_r(const time_t*, tm*);

#include <ctime>
#include <type_traits>

#include "test_macros.h"

void test() {
  std::time_t t = 0;
  std::tm tm    = {};
  std::timespec ts = {};
  ASSERT_SAME_TYPE(decltype(std::timegm(&tm)), std::time_t);
  ASSERT_SAME_TYPE(decltype(std::timespec_getres(&ts, TIME_UTC)), int);
  ASSERT_SAME_TYPE(decltype(std::gmtime_r(&t, &tm)), std::tm*);
  ASSERT_SAME_TYPE(decltype(std::localtime_r(&t, &tm)), std::tm*);
}
