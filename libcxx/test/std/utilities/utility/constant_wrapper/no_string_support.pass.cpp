//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23, c++26

// <utility>

// P4206R0: constant_wrapper is again `template<auto X, class = decltype(X)>`; the
// cw-fixed-value machinery that made string literals usable is gone.

#include <cassert>
#include <type_traits>
#include <utility>

#ifndef __cpp_lib_constant_wrapper
#  error __cpp_lib_constant_wrapper should be defined
#endif
#if __cpp_lib_constant_wrapper != 202606L
#  error __cpp_lib_constant_wrapper should have the value 202606L in C++29
#endif

// cw<5> is constant_wrapper<5, int>: the template argument is the value itself.
static_assert(std::is_same_v<decltype(std::cw<5>), const std::constant_wrapper<5, int>>);
static_assert(std::is_same_v<decltype(std::cw<5>), const std::constant_wrapper<5>>);
static_assert(std::is_same_v<std::constant_wrapper<5>::value_type, int>);
static_assert(std::is_same_v<std::constant_wrapper<5>::type, std::constant_wrapper<5>>);
static_assert(std::constant_wrapper<5>::value == 5);

// The motivating example: deduction of the value works (it did not with cw-fixed-value).
template <int I>
constexpr int deduce_int(std::constant_wrapper<I>) {
  return I;
}
static_assert(deduce_int(std::cw<5>) == 5);

template <auto V>
constexpr auto deduce_auto(std::constant_wrapper<V>) {
  return V;
}
static_assert(deduce_auto(std::cw<7u>) == 7u);
static_assert(std::is_same_v<decltype(deduce_auto(std::cw<7u>)), unsigned>);

// The second template parameter is no longer needed to constrain the type.
template <int I>
constexpr int deduce_int_typed(std::constant_wrapper<I, int>) {
  return I;
}
static_assert(deduce_int_typed(std::cw<3>) == 3);
template <class T>
concept CanDeduceInt = requires(T t) { deduce_int_typed(t); };
static_assert(CanDeduceInt<decltype(std::cw<3>)>);
static_assert(!CanDeduceInt<decltype(std::cw<3u>)>);

// A class type value is stored as a template parameter object.
struct S {
  int i;
};
constexpr auto cs = std::cw<S{4}>;
static_assert(std::is_same_v<std::remove_cvref_t<decltype(cs.value)>, S>);
static_assert(cs.value.i == 4);
static_assert(std::is_same_v<decltype(cs)::value_type, S>);

// Address uniqueness: the wrapper's value is the template parameter object itself.
template <auto V>
constexpr const void* foo() {
  return &V;
}
template <auto V>
constexpr const void* bar(std::constant_wrapper<V>) {
  return &std::constant_wrapper<V>::value;
}
static_assert(foo<S{4}>() == bar(std::cw<S{4}>));

// The operators still preserve constness.
static_assert(std::is_same_v<decltype(std::cw<5> + std::cw<3>), std::constant_wrapper<8>>);

int main(int, char**) { return 0; }
