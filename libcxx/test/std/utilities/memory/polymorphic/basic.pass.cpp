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
#include <vector>

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
    // The forwarding and in_place_type_t constructors require
    // derived_from<U, T>; for a non-class T like int, is_base_of_v<int,int>
    // is false (is_base_of requires class types), so polymorphic<int> can
    // only be default-constructed, not constructed from a bare int -- this
    // matches the standard's intent that polymorphic exists for class
    // hierarchies, not scalars.
    std::polymorphic<int> p; // default ctor: requires T default- and copy-constructible
    assert(*p == 0);
    *p = 8;
    assert(*p == 8);
  }

  {
    // in_place_type_t + initializer_list constructor.
    struct Holder : Base {
      std::vector<int> data;
      Holder(std::initializer_list<int> il) : data(il) {}
      int f() const override { return static_cast<int>(data.size()); }
    };
    std::polymorphic<Base> p(std::in_place_type<Holder>, {1, 2, 3, 4});
    assert(p->f() == 4);
  }

  {
    // Allocator-extended construction.
    std::allocator<int> alloc;
    std::polymorphic<Base> p(std::allocator_arg, alloc, std::in_place_type<Derived>, 3);
    assert(p->f() == 3 && p.get_allocator() == alloc);

    std::polymorphic<Base> p2(std::allocator_arg, alloc, p);
    assert(p2->f() == 3);
  }

  {
    std::polymorphic<Base> a(std::in_place_type<Derived>, 1);
    std::polymorphic<Base> b(std::in_place_type<Derived>, 2);
    a.swap(b);
    assert(a->f() == 2 && b->f() == 1);
    swap(a, b);
    assert(a->f() == 1 && b->f() == 2);
  }

  return 0;
}
