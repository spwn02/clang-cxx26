//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17

// <numeric>

// template<class ForwardIterator, class T> void iota(ForwardIterator first, ForwardIterator last, T value);
// Mandates: T is convertible to ForwardIterator's value type.

#include <numeric>

struct Seed {
  Seed& operator++() { return *this; }
};

struct Value {
  Value& operator=(Seed) { return *this; }
};

void test() {
  Value values[1];
  // Assignment is valid, but Seed is not implicitly convertible to Value.
  // expected-error@*:* {{Mandates: T is convertible to ForwardIterator's value type}}
  std::iota(values, values + 1, Seed{});
}
