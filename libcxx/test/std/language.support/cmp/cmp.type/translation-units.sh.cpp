//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: gcc-15

// RUN: %{cxx} %{compile_flags} -DFIRST_TU -c %s -o %t.first.o
// RUN: %{cxx} %{compile_flags} -c %s -o %t.second.o
// RUN: %{cxx} %{link_flags} %t.first.o %t.second.o -o %t.exe
// RUN: %{exec} %t.exe

// The two translation units declare the same types in opposite orders and
// include their headers in opposite orders. Every pair must order identically.
#ifdef FIRST_TU
#  include <array>
#  include <compare>
namespace left { struct A {}; enum class E {}; }
namespace right { struct B {}; union U { int n; }; }
struct Inc;
template <class> struct Box {};
#else
#  include <compare>
#  include <array>
template <class> struct Box {};
struct Inc;
namespace right { union U { int n; }; struct B {}; }
namespace left { enum class E {}; struct A {}; }
#endif

#include <cassert>

template <class... T> struct type_list {};
using types = type_list<int, const int, int&, char, void, left::A, right::B, left::E, right::U, Inc, Box<int>>;

template <class T, class... U>
constexpr auto row(type_list<U...>) {
  return std::array{(std::type_order_v<T, U> < 0 ? -1 : std::type_order_v<T, U> > 0 ? 1 : 0)...};
}

template <class... T>
constexpr auto table(type_list<T...> t) {
  return std::array{row<T>(t)...};
}

using result = decltype(table(types{}));
constexpr result local_results = table(types{});
#ifdef FIRST_TU
result first() { return local_results; }
#else
result first();
int main(int, char**) { assert(first() == local_results); }
#endif
