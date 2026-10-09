//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17

// <experimental/scope>
//
// [scopeguard.uniqueres] of the Library Fundamentals TS, version 3 (P0052R10):
// std::experimental::unique_resource and make_unique_resource_checked.

#include <experimental/scope>

#include <cassert>
#include <functional>
#include <type_traits>
#include <utility>

#include "test_macros.h"

namespace ex = std::experimental;

static int released = 0;
static int last = -1;
void release_int(int v) {
  ++released;
  last = v;
}

struct Deleter {
  int* count;
  void operator()(int) const { ++*count; }
};

int main(int, char**) {
  // Default state: nothing to release.
  {
    released = 0;
    ex::unique_resource<int, Deleter> r;
    (void)r;
  }

  // Destruction calls the deleter once; release() cancels it; reset() calls it immediately.
  {
    released = 0;
    { ex::unique_resource r(3, release_int); }
    assert(released == 1 && last == 3);
    { ex::unique_resource r(4, release_int); r.release(); }
    assert(released == 1);
    {
      ex::unique_resource r(5, release_int);
      r.reset();
      assert(released == 2 && last == 5);
    }
    assert(released == 2);
    {
      ex::unique_resource r(6, release_int);
      r.reset(7); // releases 6, owns 7
      assert(released == 3 && last == 6);
      assert(r.get() == 7);
    }
    assert(released == 4 && last == 7);
  }

  // Observers and pointer access.
  {
    int value = 42;
    int count = 0;
    ex::unique_resource<int*, void (*)(int*)> r(&value, [](int*) {});
    r.release();
    assert(r.get() == &value);
    assert(*r == 42);
    assert(r.operator->() == &value);
    static_assert(std::is_same_v<decltype(*r), int&>);
    ex::unique_resource<int, Deleter> n(1, Deleter{&count});
    n.release();
    assert(n.get() == 1);
    assert(n.get_deleter().count == &count);
  }

  // Move construction and assignment transfer ownership.
  {
    released = 0;
    {
      ex::unique_resource a(8, release_int);
      ex::unique_resource b(std::move(a));
      assert(released == 0);
    }
    assert(released == 1 && last == 8);
    {
      ex::unique_resource a(9, release_int);
      ex::unique_resource b(10, release_int);
      b = std::move(a); // releases 10
      assert(released == 2 && last == 10);
    }
    assert(released == 3 && last == 9);
  }

  // make_unique_resource_checked does not call the deleter for an invalid resource.
  {
    released = 0;
    { auto r = ex::make_unique_resource_checked(-1, -1, release_int); (void)r; }
    assert(released == 0);
    { auto r = ex::make_unique_resource_checked(11, -1, release_int); (void)r; }
    assert(released == 1 && last == 11);
  }

  // Reference resource types are stored through reference_wrapper.
  {
    released = 0;
    int v = 12;
    {
      ex::unique_resource<int&, void (*)(int)> r(v, release_int);
      assert(&r.get() == &v);
    }
    assert(released == 1 && last == 12);
  }

  static_assert(!std::is_copy_constructible_v<ex::unique_resource<int, Deleter>>);
  static_assert(std::is_move_constructible_v<ex::unique_resource<int, Deleter>>);

  return 0;
}
