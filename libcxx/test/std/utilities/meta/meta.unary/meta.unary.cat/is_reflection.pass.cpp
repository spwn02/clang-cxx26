//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

#include <type_traits>

// <type_traits> alone must suffice, without including <meta>.
using info = decltype(^^int);
struct Incomplete;

template<class T, bool Expected>
constexpr bool check() {
  static_assert(std::is_reflection<T>::value == Expected);
  static_assert(std::is_reflection<const T>::value == Expected);
  static_assert(std::is_reflection<volatile T>::value == Expected);
  static_assert(std::is_reflection<const volatile T>::value == Expected);
  static_assert(std::is_reflection_v<T> == Expected);
  static_assert(std::is_reflection_v<const T> == Expected);
  static_assert(std::is_reflection_v<volatile T> == Expected);
  static_assert(std::is_reflection_v<const volatile T> == Expected);
  static_assert(std::is_base_of_v<std::true_type, std::is_reflection<info>>);
  return true;
}
static_assert(check<info, true>());
static_assert(check<int, false>());
static_assert(check<void, false>());
static_assert(check<Incomplete, false>());
static_assert(check<info&, false>());
static_assert(check<info&&, false>());

int main(int, char**) { return 0; }
