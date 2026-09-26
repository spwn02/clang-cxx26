//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17

// <type_traits>

// template<class T, class U> struct is_layout_compatible;
// template<class T, class U> inline constexpr bool is_layout_compatible_v;

#include <type_traits>

#ifndef __cpp_lib_is_layout_compatible
#  error "__cpp_lib_is_layout_compatible should be defined"
#endif
static_assert(__cpp_lib_is_layout_compatible == 201907L);

struct Empty {};
struct Empty2 {};
struct S1 {
  int a;
  double b;
};
struct S2 {
  int x;
  double y;
};
struct S3 {
  double b;
  int a;
};
class WithPrivate {
  [[maybe_unused]] int a;
  [[maybe_unused]] double b;
};
struct NotStdLayout : S1 {
  int extra;
};
enum E1 : int { e1 };
enum class E2 : int { e2 };
enum E3 : long { e3 };
enum E4 : unsigned { e4 };
union U1 {
  int i;
  char c;
};
union U2 {
  char d;
  int j;
};

template <class T, class U, bool expected>
constexpr void check() {
  static_assert(std::is_layout_compatible<T, U>::value == std::is_layout_compatible_v<T, U>);
  static_assert(std::is_layout_compatible_v<T, U> == expected);
  static_assert(std::is_layout_compatible<T, U>::value == expected);
  static_assert(std::is_base_of_v<std::bool_constant<expected>, std::is_layout_compatible<T, U>>);
  static_assert(std::is_layout_compatible_v<T, U> == std::is_layout_compatible_v<U, T>);
}

int main(int, char**) {
  // Fundamental types: layout-compatible only with themselves (ignoring cv-qualification).
  check<int, int, true>();
  check<int, const int, true>();
  check<volatile int, const int, true>();
  check<int, unsigned, false>();
  check<int, long, false>();
  check<int, float, false>();
  check<char, signed char, false>();
  check<void, void, true>();
  check<void, const void, true>();
  check<void, int, false>();
  check<int*, int*, true>();
  check<int*, long*, false>();
  check<int (*)(), int (*)(), true>();
  check<int[3], int[3], true>();
  check<int[3], int[4], false>();
  check<int[3], const int[3], true>();
  check<int[], int[], true>();
  check<int[], int[3], false>();

  // Enumerations with the same underlying type.
  check<E1, E2, true>();
  check<E1, E3, false>();
  check<E1, E4, false>();
  check<E2, E2, true>();

  // Classes.
  check<Empty, Empty2, true>();
  check<S1, S2, true>();
  check<S1, S3, false>();
  check<S1, WithPrivate, true>(); // access control does not matter
  check<S1, NotStdLayout, false>();
  check<NotStdLayout, NotStdLayout, true>(); // the same type is always layout-compatible with itself
  check<S1, S1, true>();
  check<U1, U2, true>();
  check<S1, U1, false>();

  return 0;
}
