//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// constexpr bool has_error() const noexcept;   // P3798R1

#include <cassert>
#include <expected>

#include "test_macros.h"

template <class T>
concept HasErrorNoexcept = requires(T t) {
  { t.has_error() } noexcept;
};

static_assert(HasErrorNoexcept<std::expected<int, int>>);
static_assert(HasErrorNoexcept<const std::expected<int, int>>);
static_assert(HasErrorNoexcept<std::expected<void, int>>);

constexpr bool test() {
  {
    const std::expected<int, int> e(5);
    assert(!e.has_error());
    assert(e.has_value());
  }
  {
    const std::expected<int, int> e(std::unexpect, 5);
    assert(e.has_error());
    assert(!e.has_value());
  }
  {
    const std::expected<void, int> e;
    assert(!e.has_error());
  }
  {
    const std::expected<void, int> e(std::unexpect, 5);
    assert(e.has_error());
  }
  return true;
}

int main(int, char**) {
  test();
  static_assert(test());
  return 0;
}
