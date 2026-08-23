//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// XFAIL: FROZEN-CXX03-HEADERS-FIXME

// <atomic>

// template <class T>
// struct atomic;

// P3323R1: The program is ill-formed if same_as<T, remove_cv_t<T>> is false -- std::atomic<T> is
// restricted to cv-unqualified T (atomic<volatile int> is served by volatile atomic<int> instead).

#include <atomic>

void f() {
  int x = 0;
  // expected-error@*:* {{std::atomic<T> requires that 'T' be cv-unqualified}}
  std::atomic<const int> a(x);
}

// atomic<volatile int> isn't exercised here: forming the primary template's other (pre-existing,
// unrelated to this mandate) volatile-qualified member overloads for a volatile _Tp triggers C++20's
// unrelated "volatile-qualified parameter type is deprecated" warning ([depr.volatile.type]) on top of
// the static_assert below, which this test isn't about.

void h() {
  double x = 0;
  // expected-error@*:* {{std::atomic<T> requires that 'T' be cv-unqualified}}
  std::atomic<const double> a(x);
}
