//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

// <generator>

// template<class Ref, class V = void, class Allocator = void>
// class generator;

#include <generator>

#include <cassert>
#include <string>
#include <vector>

#include "test_macros.h"

// co_yield of a plain by-value lvalue: `yielded` for generator<int> is
// `int&&` (a reference type per [coro.generator.overview]), so this exercises
// the required lvalue-to-rvalue-reference yield_value overload
// ([coro.generator.promise]), not just the "obvious" primary overload.
static std::generator<int> ints(int n) {
  for (int i = 0; i < n; ++i)
    co_yield i;
}

// Reference-yielding generator: exercises that the stored `void*` state
// preserves constness correctly on the read side even though it can't be
// typed as const itself (it's shared across differently-cv-qualified
// `yielded` instantiations).
static std::generator<const std::string&> strings(const std::vector<std::string>& v) {
  for (const std::string& s : v)
    co_yield s;
}

// By-value string generator: mixes a co_yield of a prvalue temporary and a
// co_yield of a named local lvalue.
static std::generator<std::string> owned_strings() {
  co_yield std::string("a");
  std::string s = "b";
  co_yield s;
}

// Nested generator via ranges::elements_of.
static std::generator<int> nested() {
  co_yield 100;
  co_yield std::ranges::elements_of(ints(3));
  co_yield 200;
}

struct MyException {};

static std::generator<int> throws() {
  co_yield 1;
  throw MyException();
  co_yield 2; // never reached
}

static void test_basic_by_value() {
  std::vector<int> v;
  for (int x : ints(5))
    v.push_back(x);
  assert((v == std::vector<int>{0, 1, 2, 3, 4}));
}

static void test_reference_yield() {
  std::vector<std::string> v{"x", "y", "z"};
  std::size_t n = 0;
  for (const std::string& s : strings(v)) {
    assert(&s == &v[n]); // must be the *same* object, not a copy
    ++n;
  }
  assert(n == 3);
}

static void test_owned_by_value_strings() {
  std::vector<std::string> out;
  for (auto&& s : owned_strings())
    out.push_back(s);
  assert((out == std::vector<std::string>{"a", "b"}));
}

static void test_nested_elements_of() {
  std::vector<int> v;
  for (int x : nested())
    v.push_back(x);
  assert((v == std::vector<int>{100, 0, 1, 2, 200}));
}

static void test_exception_propagation() {
  auto g  = throws();
  auto it = g.begin();
  assert(*it == 1);
  bool caught = false;
  try {
    ++it;
  } catch (const MyException&) {
    caught = true;
  }
  assert(caught);
}

static void test_move_only() {
  static_assert(!std::is_copy_constructible_v<std::generator<int>>);
  static_assert(std::is_move_constructible_v<std::generator<int>>);

  auto g  = ints(3);
  auto g2 = std::move(g);
  std::vector<int> v;
  for (int x : g2)
    v.push_back(x);
  assert((v == std::vector<int>{0, 1, 2}));
}

static void test_input_range() {
  static_assert(std::ranges::input_range<std::generator<int>>);
  static_assert(!std::ranges::forward_range<std::generator<int>>);
}

int main(int, char**) {
  test_basic_by_value();
  test_reference_yield();
  test_owned_by_value_strings();
  test_nested_elements_of();
  test_exception_propagation();
  test_move_only();
  test_input_range();

  return 0;
}
