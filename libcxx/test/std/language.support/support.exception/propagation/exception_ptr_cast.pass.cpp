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

#if TEST_STD_VER >= 26
struct CBase { int base = 5; };
struct CDerived : CBase { int derived = 9; };
struct Tracked {
  int value;
  int* destructions;
  constexpr Tracked(int v, int* d) : value(v), destructions(d) {}
  constexpr Tracked(const Tracked& other) : value(other.value), destructions(other.destructions) {}
  constexpr ~Tracked() { ++*destructions; }
};

constexpr bool test_constexpr_exception_ptr_identity() {
  auto p = std::make_exception_ptr(CDerived{});
  auto q = p;
  auto derived = std::exception_ptr_cast<CDerived>(p);
  auto base = std::exception_ptr_cast<CBase>(q);
  if (!derived || !base || derived->derived != 9 || base->base != 5 || p != q)
    return false;
  p = nullptr;
  auto moved = std::move(q);
  return !p && !q && moved &&
         std::exception_ptr_cast<CDerived>(moved)->derived == 9;
}
static_assert(test_constexpr_exception_ptr_identity());

constexpr bool test_constexpr_exception_ptr_rethrow() {
  auto p = std::make_exception_ptr(41);
  try {
    try {
      std::rethrow_exception(p);
    } catch (double) {
      return false;
    }
  } catch (int value) {
    return value == 41 && *std::exception_ptr_cast<int>(p) == 41;
  }
  return false;
}
static_assert(test_constexpr_exception_ptr_rethrow());

struct MutableException {
  int value;
};
constexpr bool test_constexpr_exception_ptr_rethrow_observes_mutation() {
  auto p = std::make_exception_ptr(MutableException{1});
  try {
    try {
      std::rethrow_exception(p);
    } catch (MutableException& caught) {
      caught.value = 9;
      std::rethrow_exception(p);
    }
  } catch (MutableException caught) {
    return caught.value == 9 &&
           std::exception_ptr_cast<MutableException>(p)->value == 9;
  }
  return false;
}
static_assert(test_constexpr_exception_ptr_rethrow_observes_mutation());

struct CopyTrackedException {
  int* copies;
  int* destructions;
  constexpr CopyTrackedException(int* c, int* d) : copies(c), destructions(d) {}
  constexpr CopyTrackedException(const CopyTrackedException& other)
      : copies(other.copies), destructions(other.destructions) {
    ++*copies;
  }
  constexpr ~CopyTrackedException() { ++*destructions; }
};
constexpr bool test_constexpr_exception_ptr_copy_and_lifetime() {
  int copies = 0;
  int destructions = 0;
  {
    auto p = std::make_exception_ptr(CopyTrackedException(&copies, &destructions));
    if (copies != 1 || destructions != 1)
      return false;
    auto q = p;
    auto r = std::move(q);
    if (copies != 1 || destructions != 1 || q || !r)
      return false;
    p = nullptr;
    if (destructions != 1)
      return false;
  }
  return copies == 1 && destructions == 2;
}
static_assert(test_constexpr_exception_ptr_copy_and_lifetime());

constexpr bool test_constexpr_by_value_catch_runs_copy_constructor() {
  int copies = 0;
  int destructions = 0;
  {
    auto p = std::make_exception_ptr(CopyTrackedException(&copies, &destructions));
    if (copies != 1 || destructions != 1)
      return false;
    try {
      std::rethrow_exception(p);
    } catch (CopyTrackedException caught) {
      if (copies != 2 || caught.copies != &copies)
        return false;
    }
  }
  return copies == 2 && destructions == 3;
}
static_assert(test_constexpr_by_value_catch_runs_copy_constructor());

// std::current_exception() is not constexpr (P3842R2): the exception_ptr comes from make_exception_ptr.
constexpr bool test_constexpr_exception_ptr_last_release_in_nested_handlers() {
  int destructions = 0;
  {
    auto outer     = std::make_exception_ptr(Tracked{11, &destructions});
    const int base = destructions; // the by-value parameter of make_exception_ptr
    try {
      std::rethrow_exception(outer);
    } catch (const Tracked&) {
      auto inner = outer;
      outer      = nullptr;
      inner      = nullptr;
      if (destructions != base)
        return false;
    }
    if (destructions != base + 1)
      return false;
  }
  return destructions == 2;
}
static_assert(test_constexpr_exception_ptr_last_release_in_nested_handlers());

constexpr bool test_constexpr_exception_ptr_destruction() {
  int destructions = 0;
  {
    auto p = std::make_exception_ptr(Tracked{7, &destructions});
    auto q = p;
    if (destructions != 1 || std::exception_ptr_cast<Tracked>(q)->value != 7)
      return false;
    p = nullptr;
    if (destructions != 1)
      return false;
  }
  return destructions == 2;
}
static_assert(test_constexpr_exception_ptr_destruction());
#endif

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
