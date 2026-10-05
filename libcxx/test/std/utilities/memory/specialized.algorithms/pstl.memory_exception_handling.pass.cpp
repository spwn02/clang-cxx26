//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14
// UNSUPPORTED: no-exceptions
// `check_assertion.h` requires Unix headers and regex support.
// REQUIRES: has-unix-headers
// UNSUPPORTED: no-localization
// UNSUPPORTED: libcpp-has-no-incomplete-pstl

// [algorithms.parallel.exceptions]: an exception thrown by the construction of an element in an algorithm with an execution
// policy calls std::terminate; the elements constructed so far are destroyed before.

#include <cstddef>
#include <execution>
#include <memory>
#include <new>

#include "check_assertion.h"
#include "test_macros.h"

#if TEST_STD_VER >= 26
#  include <ranges>
#endif

struct Throwing {
  static inline int live = 0;
  static inline int budget = 1000; // how many more constructions succeed
  Throwing() { construct(); }
  Throwing(int) { construct(); }
  Throwing(const Throwing&) { construct(); }
  Throwing(Throwing&&) { construct(); }
  ~Throwing() { --live; }
  static void construct() {
    if (budget-- == 0)
      throw 1;
    ++live;
  }
};

template <class F>
void expect_terminate(F f) {
  Throwing::live   = 0;
  Throwing::budget = 2;
  EXPECT_STD_TERMINATE([&] {
    bool threw = false;
    try {
      f();
    } catch (...) {
      threw = true;
    }
    if (!threw)
      std::terminate();
  });
}

alignas(Throwing) unsigned char buffer[sizeof(Throwing) * 8];
Throwing* out() { return reinterpret_cast<Throwing*>(buffer); }

int main(int, char**) {
  const Throwing proto(1); // budget is irrelevant here: construction before the first expect_terminate
  const auto& policy = std::execution::seq;
  Throwing sources[4];

  expect_terminate([&] { std::uninitialized_default_construct(policy, out(), out() + 4); });
  expect_terminate([&] { std::uninitialized_default_construct_n(policy, out(), 4); });
  expect_terminate([&] { std::uninitialized_value_construct(policy, out(), out() + 4); });
  expect_terminate([&] { std::uninitialized_value_construct_n(policy, out(), 4); });
  expect_terminate([&] { std::uninitialized_copy(policy, sources, sources + 4, out()); });
  expect_terminate([&] { std::uninitialized_copy_n(policy, sources, 4, out()); });
  expect_terminate([&] { std::uninitialized_move(policy, sources, sources + 4, out()); });
  expect_terminate([&] { std::uninitialized_move_n(policy, sources, 4, out()); });
  expect_terminate([&] { std::uninitialized_fill(policy, out(), out() + 4, proto); });
  expect_terminate([&] { std::uninitialized_fill_n(policy, out(), 4, proto); });
#if TEST_STD_VER >= 26
  expect_terminate([&] { std::ranges::uninitialized_default_construct(policy, out(), out() + 4); });
  expect_terminate([&] { std::ranges::uninitialized_value_construct_n(policy, out(), 4); });
  expect_terminate([&] { std::ranges::uninitialized_copy(policy, sources, sources + 4, out(), out() + 4); });
  expect_terminate([&] { std::ranges::uninitialized_move_n(policy, sources, 4, out(), out() + 4); });
  expect_terminate([&] { std::ranges::uninitialized_fill(policy, out(), out() + 4, proto); });
#endif
  return 0;
}
