//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17

// <type_traits>

// template<class S1, class S2, class M1, class M2>
//   constexpr bool is_corresponding_member(M1 S1::* m1, M2 S2::* m2) noexcept;
// template<class S, class M>
//   constexpr bool is_pointer_interconvertible_with_class(M S::* m) noexcept;

#include <type_traits>

struct A {
  int a;
  char b;
  double c;
};
struct B {
  int x;
  char y;
  float z;
};
struct C {
  int p;
  long q;
};
struct WithBase : A {};
struct Virtual {
  virtual void f();
  int v;
};
union U {
  int i;
  char c;
};
struct Empty {};
struct EmptyBaseFirst : Empty {
  int m;
};
struct D {
  int first;
  int second;
};
struct Derived : D {};

// common initial sequence: int, char are compatible; double vs float end it
static_assert(std::is_corresponding_member(&A::a, &B::x));
static_assert(std::is_corresponding_member(&A::b, &B::y));
static_assert(!std::is_corresponding_member(&A::c, &B::z));
static_assert(!std::is_corresponding_member(&A::a, &B::y)); // different positions
static_assert(!std::is_corresponding_member(&A::b, &C::q)); // not layout-compatible
static_assert(std::is_corresponding_member(&A::a, &C::p));
static_assert(!std::is_corresponding_member(&A::b, &C::q));
static_assert(std::is_corresponding_member(&A::a, &A::a));
static_assert(!std::is_corresponding_member(&A::a, &A::b));

// members of a standard-layout base
static_assert(std::is_corresponding_member(&WithBase::a, &B::x));
static_assert(std::is_corresponding_member(&A::a, &WithBase::a));

// not standard-layout, null pointers, unions
static_assert(!std::is_corresponding_member(&Virtual::v, &Virtual::v));
static_assert(!std::is_corresponding_member(static_cast<int A::*>(nullptr), &B::x));
static_assert(!std::is_corresponding_member(&B::x, static_cast<int A::*>(nullptr)));
static_assert(!std::is_corresponding_member(&U::i, &U::i));

static_assert(noexcept(std::is_corresponding_member(&A::a, &B::x)));

// pointer-interconvertibility with the class
static_assert(std::is_pointer_interconvertible_with_class(&A::a));
static_assert(!std::is_pointer_interconvertible_with_class(&A::b));
static_assert(std::is_pointer_interconvertible_with_class(&U::i));
static_assert(std::is_pointer_interconvertible_with_class(&U::c));
static_assert(!std::is_pointer_interconvertible_with_class(&Virtual::v));
static_assert(!std::is_pointer_interconvertible_with_class(static_cast<int A::*>(nullptr)));
static_assert(std::is_pointer_interconvertible_with_class(&D::first));
static_assert(!std::is_pointer_interconvertible_with_class(&D::second));
static_assert(std::is_pointer_interconvertible_with_class(static_cast<int Derived::*>(&D::first))); // first base, no members in front
static_assert(!std::is_pointer_interconvertible_with_class(static_cast<int Derived::*>(&D::second)));
static_assert(std::is_pointer_interconvertible_with_class(&EmptyBaseFirst::m));
static_assert(noexcept(std::is_pointer_interconvertible_with_class(&A::a)));

int main(int, char**) { return 0; }
