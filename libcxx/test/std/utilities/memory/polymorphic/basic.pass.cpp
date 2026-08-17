//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <memory>

// Basic functional coverage of std::polymorphic: in_place_type_t
// construction of a derived type, copy clones the derived object, move
// leaves the source valueless.

#include <memory>
#include <cassert>
#include <utility>

#include "test_macros.h"

namespace {
struct Base {
  virtual ~Base()       = default;
  virtual int f() const = 0;
};
struct Derived : Base {
  int value;
  explicit Derived(int v) : value(v) {}
  int f() const override { return value; }
};
} // namespace

int main(int, char**) {
  {
    std::polymorphic<Base> p(std::in_place_type<Derived>, 42);
    assert(p->f() == 42);
    assert(!p.valueless_after_move());

    std::polymorphic<Base> p2 = p;
    assert(p2->f() == 42);

    std::polymorphic<Base> p3 = std::move(p);
    assert(p.valueless_after_move());
    assert(p3->f() == 42);
  }

  {
    std::polymorphic<int> p(7);
    assert(*p == 7);
    *p = 8;
    assert(*p == 8);
  }

  return 0;
}
