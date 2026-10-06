//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <utility>

// constant_wrapper::operator() and operator[] (P3978R3): with constant-wrapper arguments and a constant result the result is a
// constant_wrapper; otherwise the plain call/subscript is made.

#include <cassert>
#include <type_traits>
#include <utility>

struct F {
  constexpr int operator()(int a, int b) const { return a + b; }
};

struct Indexable {
  int data[4] = {10, 20, 30, 40};
  constexpr int operator[](int i) const { return data[i]; }
};

constexpr int plain(int a) { return a * 2; }
static constexpr Indexable indexable{};

constexpr bool test() {
  // all constant-wrapper arguments and a constant result: a constant_wrapper
  {
    constexpr auto r = std::cw<F{}>(std::cw<1>, std::cw<2>);
    static_assert(std::is_same_v<std::remove_cv_t<decltype(r)>, std::constant_wrapper<3>>);
    static_assert(r == 3);
  }
  // a plain argument: the invocation is made at run time
  {
    auto r = std::cw<F{}>(std::cw<1>, 2);
    static_assert(std::is_same_v<decltype(r), int>);
    assert(r == 3);
    auto s = std::cw<plain>(4);
    static_assert(std::is_same_v<decltype(s), int>);
    assert(s == 8);
  }
  // subscript
  {
    constexpr auto r = std::cw<indexable>[std::cw<2>];
    static_assert(std::is_same_v<std::remove_cv_t<decltype(r)>, std::constant_wrapper<30>>);
    int i = 3;
    auto s = std::cw<indexable>[i];
    static_assert(std::is_same_v<decltype(s), int>);
    assert(s == 40);
  }
  return true;
}

int main(int, char**) {
  test();
  static_assert(test());
  return 0;
}
