//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <memory>

// Basic functional coverage of std::indirect: construction, copy makes a
// deep copy, move leaves the source valueless, comparisons, hashing.

#include <memory>
#include <cassert>
#include <unordered_set>
#include <utility>

#include "test_macros.h"

int main(int, char**) {
  {
    std::indirect<int> a(5);
    assert(*a == 5);
    assert(!a.valueless_after_move());

    std::indirect<int> b = a;
    *b                   = 10;
    assert(*a == 5 && *b == 10);

    std::indirect<int> c = std::move(a);
    assert(a.valueless_after_move());
    assert(*c == 5);
  }

  {
    std::indirect<int> a(1), b(1), c(2);
    assert(a == b);
    assert(a != c);
    assert(a < c);

    std::hash<std::indirect<int>> h;
    assert(h(a) == h(b));

    std::unordered_set<std::indirect<int>> s;
    s.insert(std::indirect<int>(1));
    s.insert(std::indirect<int>(2));
    assert(s.size() == 2);
  }

  {
    std::indirect<int> a(std::in_place, 42);
    assert(*a == 42);
  }

  return 0;
}
