//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17

// <type_traits>

// template<class Base, class Derived> struct is_pointer_interconvertible_base_of;
// template<class Base, class Derived> inline constexpr bool is_pointer_interconvertible_base_of_v;

#include <type_traits>

struct Base {
  int x;
};
struct Derived : Base {}; // standard-layout, Base is at offset zero
struct Derived2 : Base {
  int y;
};
struct Empty {};
struct Multi : Empty, Base {}; // Empty is a base subobject of Multi, but Base is not pointer-interconvertible
struct Poly {
  virtual ~Poly() = default;
};
struct DerivedPoly : Poly {};
struct A {
  int a;
};
struct B {
  int b;
};
struct C : A, B {}; // not standard layout: two bases with data members
union U {
  int i;
};

template <class B, class D, bool expected>
constexpr void check() {
  static_assert(std::is_pointer_interconvertible_base_of<B, D>::value == std::is_pointer_interconvertible_base_of_v<B, D>);
  static_assert(std::is_pointer_interconvertible_base_of_v<B, D> == expected);
  static_assert(std::is_base_of_v<std::bool_constant<expected>, std::is_pointer_interconvertible_base_of<B, D>>);
}

int main(int, char**) {
  check<Base, Base, true>(); // a type is a pointer-interconvertible base of itself (if it is a standard-layout class)
  check<Base, Derived, true>();
  check<Base, Derived2, false>(); // Derived2 has a data member of its own next to the base one: not standard-layout
  check<Derived, Base, false>();
  check<A, C, false>();
  check<B, C, false>();
  check<Poly, DerivedPoly, false>(); // not standard-layout
  check<int, int, false>();
  check<Base, int, false>();
  check<U, U, false>(); // unions are not classes for this purpose
  return 0;
}
