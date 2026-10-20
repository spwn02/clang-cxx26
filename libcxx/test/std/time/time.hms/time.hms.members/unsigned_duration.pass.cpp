//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17

// <chrono>

// constexpr explicit hh_mm_ss(Duration d);

// LWG4274: the constructor is well-formed for a duration with an unsigned representation
// (ABS_D is -d if is-neg and d otherwise; chrono::abs requires a signed representation).

#include <cassert>
#include <chrono>
#include <ratio>

using namespace std::chrono;

constexpr bool test() {
  using U = duration<unsigned, std::ratio<1>>;
  hh_mm_ss<U> h(U(3725));
  assert(!h.is_negative());
  assert(h.hours() == hours(1));
  assert(h.minutes() == minutes(2));
  assert(h.seconds() == seconds(5));

  using V = duration<unsigned long long, std::milli>;
  hh_mm_ss<V> g(V(3725123));
  assert(g.subseconds().count() == 123);

  hh_mm_ss<seconds> n(seconds(-3725));
  assert(n.is_negative());
  assert(n.hours() == hours(1) && n.minutes() == minutes(2) && n.seconds() == seconds(5));
  return true;
}

int main(int, char**) {
  test();
  static_assert(test());
  return 0;
}
