//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <chrono>
// [time.hash]
//
// template<class Rep, class Period> struct hash<chrono::duration<Rep, Period>>;
// template<class Clock, class Duration> struct hash<chrono::time_point<Clock, Duration>>;
// template<> struct hash<chrono::day>;
// template<> struct hash<chrono::month>;
// template<> struct hash<chrono::year>;
// template<> struct hash<chrono::weekday>;
// template<> struct hash<chrono::weekday_indexed>;
// template<> struct hash<chrono::weekday_last>;
// template<> struct hash<chrono::month_day>;
// template<> struct hash<chrono::month_day_last>;
// template<> struct hash<chrono::month_weekday>;
// template<> struct hash<chrono::month_weekday_last>;
// template<> struct hash<chrono::year_month>;
// template<> struct hash<chrono::year_month_day>;
// template<> struct hash<chrono::year_month_day_last>;
// template<> struct hash<chrono::year_month_weekday>;
// template<> struct hash<chrono::year_month_weekday_last>;

#include <chrono>

#include <cassert>
#include <type_traits>
#include <unordered_set>

#include "poisoned_hash_helper.h"

using namespace std::chrono;

// [time.hash]p1: hash<duration<Rep, Period>> is enabled iff hash<Rep> is enabled.
// [time.hash]p2: hash<time_point<Clock, Duration>> is enabled iff hash<Duration> is enabled.
// [time.hash]p3: the calendar-type specializations are all enabled unconditionally.
//
// test_hash_enabled<Key>() checks the full Cpp17Hash requirement set (default-
// constructible/copyable/swappable Hash, callable on Key/cv/ref-qualified and
// implicitly-convertible-to-Key arguments, equal keys hash equal).
void testEnabled() {
  test_hash_enabled<seconds>();
  test_hash_enabled<milliseconds>();
  test_hash_enabled<sys_seconds>();
  test_hash_enabled<sys_days>();

  test_hash_enabled<day>();
  test_hash_enabled<month>();
  test_hash_enabled<year>();
  test_hash_enabled<weekday>();
  test_hash_enabled<weekday_indexed>(weekday_indexed(Monday, 1));
  test_hash_enabled<weekday_last>(weekday_last(Monday));
  test_hash_enabled<month_day>(month_day(March, day(5)));
  test_hash_enabled<month_day_last>(month_day_last(March));
  test_hash_enabled<month_weekday>(March / Monday[1]);
  test_hash_enabled<month_weekday_last>(March / Monday[last]);
  test_hash_enabled<year_month>(year(2024) / March);
  test_hash_enabled<year_month_day>(year(2024) / March / 5);
  test_hash_enabled<year_month_day_last>(year(2024) / March / last);
  test_hash_enabled<year_month_weekday>(year(2024) / March / Monday[1]);
  test_hash_enabled<year_month_weekday_last>(year(2024) / March / Monday[last]);
}

// [time.hash]p1/p2: hash<duration<Rep, Period>>/hash<time_point<Clock, Duration>>
// are disabled when hash<Rep>/hash<Duration> is disabled. `Class` (an empty
// struct with no std::hash specialization, from poisoned_hash_helper.h) is not
// itself a valid duration Rep semantically, but that's irrelevant here: only
// the type's *hashability* is under test, and class-template instantiation
// doesn't require Rep's arithmetic operations to be well-formed unless used.
void testDisabled() {
  test_hash_disabled<duration<Class>>();
  test_hash_disabled<time_point<system_clock, duration<Class>>>();
}

template <class Key>
void testEqualHashesEqual(const Key& a, const Key& b) {
  assert(a == b);
  std::hash<Key> h;
  assert(h(a) == h(b));
}

int main(int, char**) {
  testEnabled();
  testDisabled();

  testEqualHashesEqual(seconds(5), seconds(5));
  testEqualHashesEqual(milliseconds(1234), milliseconds(1234));

  testEqualHashesEqual(sys_seconds(seconds(5)), sys_seconds(seconds(5)));
  testEqualHashesEqual(sys_days(days(5)), sys_days(days(5)));

  testEqualHashesEqual(day(5), day(5));
  testEqualHashesEqual(month(3), month(3));
  testEqualHashesEqual(year(2024), year(2024));
  testEqualHashesEqual(Monday, Monday);
  testEqualHashesEqual(weekday_indexed(Monday, 1), weekday_indexed(Monday, 1));
  testEqualHashesEqual(weekday_last(Monday), weekday_last(Monday));
  testEqualHashesEqual(month_day(March, day(5)), month_day(March, day(5)));
  testEqualHashesEqual(month_day_last(March), month_day_last(March));
  testEqualHashesEqual(March / Monday[1], March / Monday[1]);
  testEqualHashesEqual(March / Monday[last], March / Monday[last]);
  testEqualHashesEqual(year(2024) / March, year(2024) / March);
  testEqualHashesEqual(year(2024) / March / 5, year(2024) / March / 5);
  testEqualHashesEqual(year(2024) / March / last, year(2024) / March / last);
  testEqualHashesEqual(year(2024) / March / Monday[1], year(2024) / March / Monday[1]);
  testEqualHashesEqual(year(2024) / March / Monday[last], year(2024) / March / Monday[last]);

  // [time.hash]p3, Note 1: hash<Key> meets Cpp17Hash even when k.ok() is false.
  {
    day invalidDay(200);
    assert(!invalidDay.ok());
    std::hash<day> h;
    assert(h(invalidDay) == h(day(200))); // does not crash / UB, and is consistent
  }
  {
    month invalidMonth(200);
    assert(!invalidMonth.ok());
    std::hash<month> h;
    assert(h(invalidMonth) == h(month(200)));
  }
  {
    year_month_day invalidYmd(year(2024), month(2), day(30)); // Feb 30 does not exist
    assert(!invalidYmd.ok());
    std::hash<year_month_day> h;
    assert(h(invalidYmd) == h(invalidYmd));
  }

  // Distinct values are not required to hash differently, but a sane
  // implementation should not collide on these trivially-distinct cases.
  assert(std::hash<day>()(day(5)) != std::hash<day>()(day(6)));
  assert(std::hash<month>()(month(3)) != std::hash<month>()(month(4)));
  assert(std::hash<year>()(year(2024)) != std::hash<year>()(year(2025)));
  assert(std::hash<year_month_day>()(year(2024) / March / 5) != std::hash<year_month_day>()(year(2024) / March / 6));

  // Usable as an unordered_set key.
  std::unordered_set<year_month_day> s;
  s.insert(year(2024) / March / 5);
  assert(s.contains(year(2024) / March / 5));
  assert(!s.contains(year(2024) / March / 6));

  return 0;
}
