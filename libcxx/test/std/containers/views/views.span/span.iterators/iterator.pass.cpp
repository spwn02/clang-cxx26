//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17

// <span>

// class iterator

#include <cassert>
#include <concepts>
#include <iterator>
#include <span>
#include <string>

#include "test_macros.h"

// Note: std::span has no const_iterator/cbegin/cend/crbegin/crend members --
// confirmed against the standard wording (verified 2026-09-12, since a prior
// version of this test speculatively guessed otherwise): P2278R4 only
// reworks the ranges::cbegin/cend (and std::cbegin/cend) customization point
// objects themselves, which work generically on span via its ordinary
// begin()/end()/rbegin()/rend() members like any other range -- span doesn't
// need its own const_iterator member type or member cbegin()/crbegin() for
// that to work correctly.

template <class T>
constexpr void test_type() {
  using C = std::span<T>;
  typename C::iterator ii1{}, ii2{};
  typename C::iterator ii4 = ii1;
  assert(ii1 == ii2);
  assert(ii1 == ii4);

  assert(!(ii1 != ii2));

  T v;
  C c{&v, 1};
  assert(c.begin() == std::begin(c));
  assert(c.rbegin() == std::rbegin(c));

  assert(c.end() == std::end(c));
  assert(c.rend() == std::rend(c));

  assert(std::begin(c) != std::end(c));
  assert(std::rbegin(c) != std::rend(c));

  // P1614 + LWG3352
  std::same_as<std::strong_ordering> decltype(auto) r1 = ii1 <=> ii2;
  assert(r1 == std::strong_ordering::equal);
}

constexpr bool test() {
  test_type<char>();
  test_type<int>();
  test_type<std::string>();

  return true;
}

int main(int, char**) {
  test();
  static_assert(test(), "");

  return 0;
}
