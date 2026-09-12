//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17

// constexpr auto size() const requires see below;

#include <cassert>
#include <concepts>
#include <limits>
#include <ranges>

#include "test_macros.h"
#include "types.h"

constexpr bool test() {
  // Both are integer like and both are less than zero.
  {
    const std::ranges::iota_view<int, int> io(-10, -5);
    assert(io.size() == 5);
  }
  {
    const std::ranges::iota_view<int, int> io(-10, -10);
    assert(io.size() == 0);
  }

  // Both are integer like and "value_" is less than zero.
  {
    const std::ranges::iota_view<int, int> io(-10, 10);
    assert(io.size() == 20);
  }
  // LWG3614: negating the most-negative representable value of an integer-like type
  // (as the pre-LWG3614 formula did) is undefined behavior. Fixed via
  // ranges::__negate_to_unsigned_like, which negates in the unsigned domain instead.
  {
    const std::ranges::iota_view<int, int> io(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
    assert(io.size() == (static_cast<unsigned>(std::numeric_limits<int>::max()) * 2u) + 1u);
  }
  {
    const std::ranges::iota_view<int, int> io(std::numeric_limits<int>::min(), 0);
    assert(io.size() == static_cast<unsigned>(std::numeric_limits<int>::max()) + 1u);
  }
  {
    const std::ranges::iota_view<int, int> io(
        std::numeric_limits<int>::min(), std::numeric_limits<int>::min() + 5);
    assert(io.size() == 5);
  }
  // Regression check for narrow integer-like types (P2278R4/LWG3614 fix interaction):
  // the negation helper must preserve the same promoted-to-unsigned result type and
  // value that the naive (but UB-prone) formulation produced for in-range inputs.
  {
    const std::ranges::iota_view<short, short> io(short(-10), short(-5));
    std::same_as<unsigned int> auto sz = io.size();
    assert(sz == 5);
  }

  // It is UB for "bound_" to be less than "value_" i.e.: iota_view<int, int> io(10, -5).

  // Both are integer like and neither less than zero.
  {
    const std::ranges::iota_view<int, int> io(10, 20);
    assert(io.size() == 10);
  }
  {
    const std::ranges::iota_view<int, int> io(10, 10);
    assert(io.size() == 0);
  }
  {
    const std::ranges::iota_view<int, int> io(0, 0);
    assert(io.size() == 0);
  }
  {
    const std::ranges::iota_view<int, int> io(0, std::numeric_limits<int>::max());
    constexpr auto imax = std::numeric_limits<int>::max();
    assert(io.size() == imax);
  }

  // Neither are integer like.
  {
    const std::ranges::iota_view<SomeInt, SomeInt> io(SomeInt(-20), SomeInt(-10));
    assert(io.size() == 10);
  }
  {
    const std::ranges::iota_view<SomeInt, SomeInt> io(SomeInt(-10), SomeInt(-10));
    assert(io.size() == 0);
  }
  {
    const std::ranges::iota_view<SomeInt, SomeInt> io(SomeInt(0), SomeInt(0));
    assert(io.size() == 0);
  }
  {
    const std::ranges::iota_view<SomeInt, SomeInt> io(SomeInt(10), SomeInt(20));
    assert(io.size() == 10);
  }
  {
    const std::ranges::iota_view<SomeInt, SomeInt> io(SomeInt(10), SomeInt(10));
    assert(io.size() == 0);
  }

  // Make sure iota_view<short, short> works properly. For details,
  // see https://llvm.org/PR67551.
  {
    static_assert(std::ranges::sized_range<std::ranges::iota_view<short, short>>);
    std::ranges::iota_view<short, short> io(10, 20);
    std::same_as<unsigned int> auto sz = io.size();
    assert(sz == 10);
  }

  return true;
}

int main(int, char**) {
  test();
  static_assert(test());

  return 0;
}
