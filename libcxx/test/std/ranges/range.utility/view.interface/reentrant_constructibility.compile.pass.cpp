//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17

// Regression test for https://github.com/spwn02/clang-cxx26/issues/112.
// A losing converting constructor must not make the view's constructibility
// query recursively instantiate itself when its move constructor is a perfect
// match.

#include <ranges>
#include <span>
#include <utility>

template <std::ranges::view V>
class View : public std::ranges::view_interface<View<V>> {
public:
  explicit View(V value) : value_(std::move(value)) {}

  auto begin() { return value_.begin(); }
  auto end() { return value_.end(); }

private:
  V value_;
};

template <class V>
View(V) -> View<V>;

template <std::ranges::view V>
class Wrapper : public std::ranges::view_interface<Wrapper<V>> {
public:
  explicit Wrapper(V value) : value_(std::move(value)) {}

  auto begin() { return value_.begin(); }
  auto end() { return value_.end(); }

private:
  V value_;
};

template <class V>
Wrapper(V) -> Wrapper<V>;

void test() {
  int value = 0;
  std::span<int> range(&value, 1);
  Wrapper wrapper{View{std::move(range)}};
}
