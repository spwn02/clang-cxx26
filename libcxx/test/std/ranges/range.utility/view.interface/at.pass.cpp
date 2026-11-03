//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23, c++26

// <ranges>

// P3052R2: view_interface::at
//
// template<random_access_range R = D> requires sized_range<R>
//   constexpr decltype(auto) at(range_difference_t<R> n);
// template<random_access_range R = const D> requires sized_range<R>
//   constexpr decltype(auto) at(range_difference_t<R> n) const;

#include <ranges>

#include <cassert>
#include <stdexcept>

#include "test_iterators.h"
#include "test_macros.h"

#if !defined(__cpp_lib_view_interface) || __cpp_lib_view_interface != 202606L
#  error "__cpp_lib_view_interface should be 202606L in C++29"
#endif

template <class T>
concept HasAt = requires(T& t) { t.at(0); };

struct SizedRandom : std::ranges::view_interface<SizedRandom> {
  int buff[4] = {10, 11, 12, 13};
  constexpr int* begin() { return buff; }
  constexpr const int* begin() const { return buff; }
  constexpr int* end() { return buff + 4; }
  constexpr const int* end() const { return buff + 4; }
};

// Random access, not sized: there is no way to bounds-check.
struct UnsizedRandom : std::ranges::view_interface<UnsizedRandom> {
  int buff[4] = {1, 2, 3, 4};
  constexpr random_access_iterator<int*> begin() { return random_access_iterator<int*>(buff); }
  constexpr sentinel_wrapper<random_access_iterator<int*>> end() {
    return sentinel_wrapper<random_access_iterator<int*>>(random_access_iterator<int*>(buff + 4));
  }
};

struct Forward : std::ranges::view_interface<Forward> {
  int buff[4] = {1, 2, 3, 4};
  constexpr forward_iterator<int*> begin() { return forward_iterator<int*>(buff); }
  constexpr forward_iterator<int*> end() { return forward_iterator<int*>(buff + 4); }
};

// The negative cases must fail for the right reason: UnsizedRandom is a random
// access range and not a sized one, Forward is not random access.
static_assert(std::ranges::random_access_range<UnsizedRandom>);
static_assert(!std::ranges::sized_range<UnsizedRandom>);
static_assert(!std::ranges::random_access_range<Forward>);
static_assert(!HasAt<decltype(std::views::iota(0))>);

static_assert(HasAt<SizedRandom>);
static_assert(HasAt<const SizedRandom>);
static_assert(!HasAt<UnsizedRandom>);
static_assert(!HasAt<Forward>);

// The parameter type is the signed difference type, as for operator[].
static_assert(std::same_as<decltype(std::declval<SizedRandom&>().at(0)), int&>);
static_assert(std::same_as<decltype(std::declval<const SizedRandom&>().at(0)), const int&>);

constexpr bool test() {
  SizedRandom v;
  assert(v.at(0) == 10);
  assert(v.at(3) == 13);
  v.at(1) = 99;
  assert(v[1] == 99);

  const SizedRandom cv{};
  assert(cv.at(2) == 12);

  // Works on standard views too.
  auto sub = std::ranges::subrange(v.begin() + 1, v.end());
  assert(sub.at(0) == 99);
  auto iota = std::views::iota(5, 9);
  assert(iota.at(3) == 8);
  return true;
}

void test_exceptions() {
#ifndef TEST_HAS_NO_EXCEPTIONS
  SizedRandom v;
  const SizedRandom cv{};
  for (long n : {-1L, 4L, 100L}) {
    try {
      (void)v.at(n);
      assert(false);
    } catch (const std::out_of_range&) {
    }
    try {
      (void)cv.at(n);
      assert(false);
    } catch (const std::out_of_range&) {
    }
  }

  // A negative index is out of range even when it would name an element
  // of the underlying sequence ([P3052R2] design).
  int base[4] = {0, 1, 2, 3};
  auto sub    = std::ranges::subrange(base + 1, base + 4);
  assert(sub[-1] == 0);
  try {
    (void)sub.at(-1);
    assert(false);
  } catch (const std::out_of_range&) {
  }
#endif
}

int main(int, char**) {
  test();
  static_assert(test());
  test_exceptions();
  return 0;
}
