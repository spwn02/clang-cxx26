//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <optional>

// template<class T> class optional;
// template<class T> class optional<T&>;

// constexpr iterator begin() noexcept;                       // C++26
// constexpr const_iterator begin() const noexcept;            // C++26
// constexpr iterator end() noexcept;                          // C++26
// constexpr const_iterator end() const noexcept;               // C++26

#include <algorithm>
#include <cassert>
#include <optional>
#include <ranges>
#include <type_traits>

#include "test_macros.h"

template <class T>
constexpr void test_engaged(std::optional<T>& opt, T const& expected) {
  assert(opt.begin() != opt.end());
  assert(std::distance(opt.begin(), opt.end()) == 1);
  assert(*opt.begin() == expected);

  std::optional<T> const& copt = opt;
  assert(copt.begin() != copt.end());
  assert(*copt.begin() == expected);

  // range-based for visits exactly the one contained element
  int count = 0;
  for (auto& elem : opt) {
    assert(elem == expected);
    ++count;
  }
  assert(count == 1);
}

template <class T>
constexpr void test_disengaged(std::optional<T>& opt) {
  assert(opt.begin() == opt.end());
  assert(std::distance(opt.begin(), opt.end()) == 0);

  std::optional<T> const& copt = opt;
  assert(copt.begin() == copt.end());

  for ([[maybe_unused]] auto& elem : opt)
    assert(false);
}

constexpr bool test() {
  // optional<T>, disengaged
  {
    std::optional<int> opt;
    test_disengaged(opt);
  }
  // optional<T>, engaged
  {
    std::optional<int> opt(42);
    test_engaged(opt, 42);

    // mutating through the iterator mutates the contained value
    *opt.begin() = 43;
    assert(*opt == 43);
  }

  // optional<T&>, disengaged
  {
    std::optional<int&> opt;
    test_disengaged(opt);
  }
  // optional<T&>, engaged
  {
    int value    = 42;
    std::optional<int&> opt(value);
    assert(opt.begin() == &value);
    assert(opt.end() == &value + 1);
    assert(std::distance(opt.begin(), opt.end()) == 1);

    // mutating through the iterator mutates the referenced object
    *opt.begin() = 43;
    assert(value == 43);
  }

  // std::ranges::begin/end and range-based for work through the range machinery
  {
    std::optional<int> opt(1);
    assert(std::ranges::begin(opt) == opt.begin());
    assert(std::ranges::end(opt) == opt.end());
    assert(std::ranges::distance(opt) == 1);

    std::optional<int> empty;
    assert(std::ranges::distance(empty) == 0);
  }

  return true;
}

int main(int, char**) {
  test();
  static_assert(test());

  return 0;
}
