//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

// template<template<class...> class C, input_range R, class... Args>
//   constexpr auto to(R&& r, Args&&... args);  // Since C++23

// An alias template with a single (non-pack) template parameter can be the template template argument.

#include <ranges>

#include <cassert>
#include <list>
#include <vector>

template <class T>
using Vec = std::vector<T>;

template <class T>
using List = std::list<T>;

int main(int, char**) {
  std::vector<int> in = {1, 2, 3, 4, 5};

  {
    auto v = std::ranges::to<Vec>(in);
    static_assert(std::is_same_v<decltype(v), std::vector<int>>);
    assert(v.size() == 5);
  }
  {
    auto v = in | std::ranges::to<Vec>();
    static_assert(std::is_same_v<decltype(v), std::vector<int>>);
    assert(v.size() == 5);
  }
  {
    auto l = std::views::iota(0, 4) | std::ranges::to<List>();
    static_assert(std::is_same_v<decltype(l), std::list<int>>);
    assert(l.size() == 4);
  }

  return 0;
}
