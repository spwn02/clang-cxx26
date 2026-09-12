//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: no-exceptions
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <exception>

// template<class E> optional<const E&> exception_ptr_cast(const exception_ptr& p) noexcept;
// template<class E> void exception_ptr_cast(const exception_ptr&&) = delete;

#include <exception>
#include <memory>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <cassert>

#include "test_macros.h"

struct Base : std::exception {};
struct Derived : Base {
  int data_ = 42;
};
struct Unrelated : std::exception {};

int main(int, char**) {
  // Null exception_ptr: always nullopt, no exception is thrown/caught.
  {
    std::exception_ptr p;
    std::optional<const Derived&> r = std::exception_ptr_cast<Derived>(p);
    assert(!r.has_value());
  }

  // Exact type match.
  {
    std::exception_ptr p = std::make_exception_ptr(Derived());
    std::optional<const Derived&> r = std::exception_ptr_cast<Derived>(p);
    assert(r.has_value());
    assert(r->data_ == 42);
  }

  // A handler of the base type also matches, per catch-matching semantics.
  {
    std::exception_ptr p = std::make_exception_ptr(Derived());
    std::optional<const Base&> r = std::exception_ptr_cast<Base>(p);
    assert(r.has_value());
  }

  // Unrelated type: no match.
  {
    std::exception_ptr p = std::make_exception_ptr(Derived());
    std::optional<const Unrelated&> r = std::exception_ptr_cast<Unrelated>(p);
    assert(!r.has_value());
  }

  // The returned reference aliases the actual stored exception object: mutations
  // observed through a second cast of the same exception_ptr should be consistent
  // (there is exactly one live exception object backing this exception_ptr).
  {
    std::exception_ptr p = std::make_exception_ptr(Derived());
    std::optional<const Derived&> r1 = std::exception_ptr_cast<Derived>(p);
    std::optional<const Derived&> r2 = std::exception_ptr_cast<Derived>(p);
    assert(r1.has_value() && r2.has_value());
    assert(std::addressof(*r1) == std::addressof(*r2));
  }

  static_assert(noexcept(std::exception_ptr_cast<Derived>(std::declval<const std::exception_ptr&>())));

  return 0;
}
