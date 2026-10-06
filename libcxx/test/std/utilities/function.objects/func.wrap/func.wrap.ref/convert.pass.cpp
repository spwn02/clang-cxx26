//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <functional>

// [func.wrap.ref.ctor] (P3961R1): constructing a function_ref from a specialization of function_ref that converts to it copies
// the bound entity and the thunk instead of referring to the other function_ref; an assignment from such a specialization
// is not deleted.

#include <cassert>
#include <functional>
#include <type_traits>

int one() { return 1; }
int two() { return 2; }
int one_noexcept() noexcept { return 1; }

int main(int, char**) {
  // noexcept(true) -> noexcept(false), same cv: a copy
  {
    std::function_ref<int() noexcept> a = one_noexcept;
    std::function_ref<int()> b(a);
    a = std::function_ref<int() noexcept>(std::cw<one_noexcept>);
    assert(b() == 1);
    static_assert(std::is_nothrow_constructible_v<std::function_ref<int()>, std::function_ref<int() noexcept>>);
  }

  // const -> non-const (int const& converts to int const& only in this direction): a copy, so re-targeting the original
  // is not seen by the new function_ref
  {
    std::function_ref<int() const> a = one;
    std::function_ref<int()> b(a);
    a = two;
    assert(b() == 1);
    assert(a() == 2);
  }

  // non-const -> const does not convert: the new function_ref refers to the other one (is-invocable-using<const T&>
  // holds), so the re-targeting is seen
  {
    std::function_ref<int()> a = one;
    std::function_ref<int() const> b(a);
    assert(b() == 1);
    a = two;
    assert(b() == 2);
  }

  // noexcept(false) -> noexcept(true) does not convert and is not invocable as noexcept: not constructible
  static_assert(!std::is_constructible_v<std::function_ref<int() noexcept>, std::function_ref<int()>>);

  // assignments from a convertible specialization are not deleted, other callables stay deleted
  static_assert(std::is_assignable_v<std::function_ref<int()>&, std::function_ref<int() noexcept>>);
  static_assert(std::is_assignable_v<std::function_ref<int()>&, std::function_ref<int() const>>);
  static_assert(!std::is_assignable_v<std::function_ref<int() const>&, std::function_ref<int()>>);
  static_assert(!std::is_assignable_v<std::function_ref<int()>&, std::function_ref<long()>>);
  return 0;
}
