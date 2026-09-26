//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

// <tuple>

// P2165R4: converting constructors and assignment operators of tuple from tuple-like objects
//
// template<tuple-like UTuple> constexpr explicit(see below) tuple(UTuple&&);
// template<class Alloc, tuple-like UTuple> constexpr explicit(see below) tuple(allocator_arg_t, const Alloc&, UTuple&&);
// template<tuple-like UTuple> constexpr tuple& operator=(UTuple&&);
// template<tuple-like UTuple> constexpr const tuple& operator=(UTuple&&) const;

#include <array>
#include <cassert>
#include <memory>
#include <ranges>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

struct Explicit {
  constexpr explicit Explicit(int v) : value(v) {}
  int value;
};
struct FromArray {
  FromArray(std::array<int, 1>) {}
};

// construction from std::array
static_assert(std::is_constructible_v<std::tuple<int, int>, std::array<int, 2>>);
static_assert(std::is_constructible_v<std::tuple<int, int, int>, std::array<int, 3>>);
static_assert(std::is_constructible_v<std::tuple<long, double>, const std::array<int, 2>&>);
static_assert(!std::is_constructible_v<std::tuple<int, int>, std::array<int, 3>>); // size mismatch
static_assert(!std::is_constructible_v<std::tuple<int, int>, std::array<int*, 2>>); // elements not constructible
// pair-likes keep working
static_assert(std::is_constructible_v<std::tuple<long, double>, std::pair<int, float>>);

// conditionally explicit
static_assert(std::is_convertible_v<std::array<int, 2>, std::tuple<int, long>>);
static_assert(std::is_constructible_v<std::tuple<Explicit, int>, std::array<int, 2>>);
static_assert(!std::is_convertible_v<std::array<int, 2>, std::tuple<Explicit, int>>);

// A one-element tuple prefers the converting constructor from a single value.
static_assert(std::is_constructible_v<std::tuple<FromArray>, std::array<int, 1>>);
static_assert(std::is_constructible_v<std::tuple<int>, std::array<int, 1>>);

// assignment
static_assert(std::is_assignable_v<std::tuple<int, int>&, std::array<int, 2>>);
static_assert(std::is_assignable_v<std::tuple<int, int>&, std::pair<long, long>>);
static_assert(!std::is_assignable_v<std::tuple<int, int>&, std::array<int, 3>>);
static_assert(!std::is_assignable_v<std::tuple<int, int>&, std::array<int*, 2>>);
static_assert(!std::is_assignable_v<std::tuple<int, int>&, std::ranges::subrange<int*>>); // a subrange is not accepted
static_assert(std::is_assignable_v<const std::tuple<int&, int&>&, std::array<int, 2>>);
static_assert(!std::is_assignable_v<const std::tuple<int, int>&, std::array<int, 2>>);

constexpr bool test() {
  {
    std::tuple<int, int, int> t = std::array<int, 3>{1, 2, 3};
    assert(std::get<0>(t) == 1 && std::get<1>(t) == 2 && std::get<2>(t) == 3);
  }
  {
    std::tuple<long, long> t(std::array<int, 2>{4, 5});
    assert(std::get<0>(t) == 4 && std::get<1>(t) == 5);
    t = std::array<int, 2>{7, 8};
    assert(std::get<0>(t) == 7 && std::get<1>(t) == 8);
    t = std::pair<int, int>{9, 10};
    assert(std::get<0>(t) == 9 && std::get<1>(t) == 10);
  }
  {
    std::tuple<Explicit, int> t(std::array<int, 2>{1, 2});
    assert(std::get<0>(t).value == 1 && std::get<1>(t) == 2);
  }
  {
    int a = 0, b = 0;
    const std::tuple<int&, int&> refs(a, b);
    refs = std::array<int, 2>{5, 6};
    assert(a == 5 && b == 6);
  }
  {
    // the elements are taken from the tuple-like with the value category of the tuple-like
    std::array<std::unique_ptr<int>, 2> arr{std::make_unique<int>(1), std::make_unique<int>(2)};
    std::tuple<std::unique_ptr<int>, std::unique_ptr<int>> t(std::move(arr));
    assert(*std::get<0>(t) == 1 && *std::get<1>(t) == 2);
    assert(arr[0] == nullptr && arr[1] == nullptr);
  }
  return true;
}

int main(int, char**) {
  test();
  static_assert(test());
  {
    std::tuple<int, int> t(std::allocator_arg, std::allocator<int>{}, std::array<int, 2>{1, 2});
    assert(std::get<0>(t) == 1 && std::get<1>(t) == 2);
    std::tuple<std::string, std::string> s(std::allocator_arg, std::allocator<char>{},
                                            std::array<const char*, 2>{"a", "b"});
    assert(std::get<0>(s) == "a" && std::get<1>(s) == "b");
  }
  return 0;
}
