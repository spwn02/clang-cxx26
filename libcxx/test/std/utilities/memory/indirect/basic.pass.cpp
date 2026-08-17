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
#include <vector>

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

  {
    std::indirect<std::vector<int>> a(std::in_place, {1, 2, 3});
    assert(a->size() == 3 && (*a)[1] == 2);
  }

  {
    // Perfect-forwarded assignment: constructs into a valueless indirect,
    // assigns in place into a non-valueless one.
    std::indirect<int> a(5);
    a = 10;
    assert(*a == 10);

    std::indirect<int> b = std::move(a);
    assert(a.valueless_after_move());
    a = 7; // a was valueless; this must construct, not assign through *a
    assert(*a == 7 && !a.valueless_after_move());
  }

  {
    // Comparisons against a bare value (and against another indirect) are
    // well-defined even when the indirect is valueless -- unlike operator*
    // and operator->, which are precondition-narrow (UB if valueless).
    std::indirect<int> a(5);
    std::indirect<int> moved = std::move(a);
    assert(a.valueless_after_move());
    assert(!(a == 5));
    assert((a <=> 5) == std::strong_ordering::less);

    std::indirect<int> b(1);
    std::indirect<int> b_holder = std::move(b); // b_holder now holds 1; b is valueless
    assert(a == b);                             // both a and b are valueless -> equal
    assert(a != b_holder);                      // a valueless, b_holder holds 1 -> not equal
  }

  {
    std::allocator<int> alloc;
    std::indirect<int> a(std::allocator_arg, alloc, 42);
    assert(*a == 42 && a.get_allocator() == alloc);

    std::indirect<int> b(std::allocator_arg, alloc, a);
    assert(*b == 42);

    std::indirect<int> c(std::allocator_arg, alloc, std::move(b));
    assert(*c == 42 && b.valueless_after_move());
  }

  {
    std::indirect<int> a(1), b(2);
    a.swap(b);
    assert(*a == 2 && *b == 1);
    swap(a, b);
    assert(*a == 1 && *b == 2);
  }

  return 0;
}
