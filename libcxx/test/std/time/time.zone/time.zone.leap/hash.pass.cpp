//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: no-filesystem, no-localization, no-tzdb

// XFAIL: libcpp-has-no-experimental-tzdb
// XFAIL: availability-tzdb-missing

// <chrono>
// [time.hash]
//
// template<> struct hash<chrono::leap_second>;

#include <chrono>

#include <cassert>
#include <unordered_set>

#include "poisoned_hash_helper.h"
#include "test_chrono_leap_second.h"

int main(int, char**) {
  using namespace std::chrono;

  auto ls1 = test_leap_second_create(sys_seconds(sys_days(year(2016) / January / 1)) - seconds(1), seconds(1));
  auto ls2 = test_leap_second_create(sys_seconds(sys_days(year(2016) / January / 1)) - seconds(1), seconds(1));
  auto ls3 = test_leap_second_create(sys_seconds(sys_days(year(2015) / July / 1)) - seconds(1), seconds(1));

  // operator== compares only date(), not value() -- a same-date/different-value
  // pair must still be == (and therefore hash-equal), so a hash implementation
  // that folds in value() cannot pass this.
  auto ls1DifferentValue =
      test_leap_second_create(sys_seconds(sys_days(year(2016) / January / 1)) - seconds(1), seconds(2));

  test_hash_enabled<leap_second>(ls1);

  assert(ls1 == ls2);
  assert(ls1 != ls3);
  assert(ls1 == ls1DifferentValue);
  assert(ls1.value() != ls1DifferentValue.value());

  std::hash<leap_second> h;
  assert(h(ls1) == h(ls2));
  assert(h(ls1) == h(ls1DifferentValue));

  std::unordered_set<leap_second, std::hash<leap_second>> s;
  s.insert(ls1);
  assert(s.contains(ls2));
  assert(!s.contains(ls3));

  return 0;
}
