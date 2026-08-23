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
// template<class Duration, class TimeZonePtr> struct hash<chrono::zoned_time<Duration, TimeZonePtr>>;
//
// The specialization is enabled iff hash<Duration> and hash<TimeZonePtr> are enabled.

#include <chrono>

#include <cassert>
#include <unordered_set>

#include "poisoned_hash_helper.h"

int main(int, char**) {
  using namespace std::chrono;

  zoned_time<seconds> zt1{"UTC", sys_seconds{seconds{100}}};
  zoned_time<seconds> zt2{"UTC", sys_seconds{seconds{100}}};
  zoned_time<seconds> zt3{"UTC", sys_seconds{seconds{200}}};

  test_hash_enabled<zoned_time<seconds>>(zt1);

  assert(zt1 == zt2);
  assert(!(zt1 == zt3));

  std::hash<zoned_time<seconds>> h;
  assert(h(zt1) == h(zt2));

  std::unordered_set<zoned_time<seconds>, std::hash<zoned_time<seconds>>> s;
  s.insert(zt1);
  assert(s.contains(zt2));
  assert(!s.contains(zt3));

  return 0;
}
