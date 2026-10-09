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
// [scopeguard.exit] of the Library Fundamentals TS, version 3 (P0052R10):
// std::experimental::scope_exit, scope_fail and scope_success.

#include <experimental/scope>

#include <cassert>
#include <exception>
#include <type_traits>
#include <utility>

#include "test_macros.h"

namespace ex = std::experimental;

#ifndef __cpp_lib_experimental_scope
#  error "__cpp_lib_experimental_scope is not defined"
#elif __cpp_lib_experimental_scope != 201902L
#  error "__cpp_lib_experimental_scope has the wrong value"
#endif

struct Thrower {
  int* calls;
  void operator()() const { ++*calls; }
};

struct ThrowsOnCopy {
  int* calls;
  ThrowsOnCopy(int* c) : calls(c) {}
  ThrowsOnCopy(const ThrowsOnCopy&) { throw 1; }
  void operator()() const { ++*calls; }
};

int free_calls = 0;
void bump() { ++free_calls; }

int main(int, char**) {
  // scope_exit: called on normal exit and when an exception is propagating; release() cancels it.
  {
    int calls = 0;
    { ex::scope_exit g([&] { ++calls; }); }
    assert(calls == 1);
    {
      ex::scope_exit g([&] { ++calls; });
      g.release();
    }
    assert(calls == 1);
#ifndef TEST_HAS_NO_EXCEPTIONS
    try {
      ex::scope_exit g([&] { ++calls; });
      throw 1;
    } catch (int) {
    }
    assert(calls == 2);
#endif
  }

  // scope_success: only when no new exception is in flight.
  {
    int calls = 0;
    { ex::scope_success g([&] { ++calls; }); }
    assert(calls == 1);
#ifndef TEST_HAS_NO_EXCEPTIONS
    try {
      ex::scope_success g([&] { ++calls; });
      throw 1;
    } catch (int) {
    }
    assert(calls == 1);
#endif
  }

  // scope_fail: only when a new exception is in flight.
  {
    int calls = 0;
    { ex::scope_fail g([&] { ++calls; }); }
    assert(calls == 0);
#ifndef TEST_HAS_NO_EXCEPTIONS
    try {
      ex::scope_fail g([&] { ++calls; });
      throw 1;
    } catch (int) {
    }
    assert(calls == 1);
#endif
  }

  // Class template argument deduction, function references, lvalue function objects.
  {
    static_assert(std::is_same_v<decltype(ex::scope_exit(Thrower{nullptr})), ex::scope_exit<Thrower>>);
    static_assert(std::is_same_v<decltype(ex::scope_fail(Thrower{nullptr})), ex::scope_fail<Thrower>>);
    static_assert(std::is_same_v<decltype(ex::scope_success(Thrower{nullptr})), ex::scope_success<Thrower>>);
    { ex::scope_exit<void (&)()> g(bump); }
    assert(free_calls == 1);
    int calls = 0;
    Thrower t{&calls};
    { ex::scope_exit<Thrower&> g(t); }
    assert(calls == 1);
  }

  // Move construction transfers the obligation: the source is released.
  {
    int calls = 0;
    {
      ex::scope_exit a([&] { ++calls; });
      ex::scope_exit b(std::move(a));
    }
    assert(calls == 1);
    static_assert(!std::is_copy_constructible_v<ex::scope_exit<Thrower>>);
    static_assert(!std::is_copy_assignable_v<ex::scope_exit<Thrower>>);
    static_assert(!std::is_move_assignable_v<ex::scope_exit<Thrower>>);
    static_assert(std::is_nothrow_destructible_v<ex::scope_exit<Thrower>>);
    static_assert(std::is_nothrow_destructible_v<ex::scope_fail<Thrower>>);
  }

  // scope_exit and scope_fail call the function when initializing the stored copy throws.
#ifndef TEST_HAS_NO_EXCEPTIONS
  {
    int calls = 0;
    ThrowsOnCopy f(&calls);
    try {
      ex::scope_exit<ThrowsOnCopy> g(f); // copies f; the copy throws
      assert(false);
    } catch (int) {
    }
    assert(calls == 1);
    try {
      ex::scope_success<ThrowsOnCopy> g(f);
      assert(false);
    } catch (int) {
    }
    assert(calls == 1); // scope_success does not call it
  }
#endif

  // Constraints: not constructible from the guard itself or from a type EF is not constructible from.
  {
    static_assert(!std::is_constructible_v<ex::scope_exit<Thrower>, int>);
    static_assert(std::is_constructible_v<ex::scope_exit<Thrower>, Thrower>);
    static_assert(std::is_constructible_v<ex::scope_exit<Thrower>, Thrower&>);
  }

  return 0;
}
