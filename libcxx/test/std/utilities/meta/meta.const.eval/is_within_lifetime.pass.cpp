//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <type_traits>

// template<class U = void, class T>
//   consteval bool is_within_lifetime(const T* p) noexcept; // C++26

#include <type_traits>

#include "test_macros.h"

#ifndef __cpp_lib_is_within_lifetime
#  if TEST_HAS_BUILTIN(__builtin_is_within_lifetime)
#    error __cpp_lib_is_within_lifetime should be defined
#  endif
#else

// Test the signature
static_assert(noexcept(std::is_within_lifetime((const int*)nullptr)));
static_assert(std::is_same_v<decltype(std::is_within_lifetime((const int*)nullptr)), bool>);

// Per [meta.const.eval]p4, a call to `is_within_lifetime` is ill-formed
// during the evaluation of an expression E as a core constant expression
// unless p points to an object usable in constant expressions or whose
// complete object's lifetime began within E. Empirically (confirmed by
// direct compiler probing, not inferred from the wording alone), E here is
// scoped to a *consteval* (immediate-function) invocation boundary: an
// ordinary `constexpr` helper function evaluated only incidentally as part
// of a `static_assert` does not count, even though the overall
// `static_assert` expression is manifestly constant-evaluated. Every helper
// below that creates the object it inspects must therefore itself be
// `consteval`, not merely `constexpr`, matching the pattern used by
// clang/test/SemaCXX/builtin-is-within-lifetime.cpp for the underlying
// builtin.

consteval bool test_scalar() {
  int i = 0;
  return std::is_within_lifetime(&i);
}
static_assert(test_scalar());

consteval bool test_union_alternative() {
  union {
    int i;
    char c;
  } u{.i = 0};
  if (!std::is_within_lifetime(&u.i))
    return false;
  if (std::is_within_lifetime(&u.c))
    return false;
  u.c = '\0'; // begins the lifetime of the `char` alternative
  if (std::is_within_lifetime(&u.i))
    return false;
  if (!std::is_within_lifetime(&u.c))
    return false;
  return true;
}
static_assert(test_union_alternative());

struct TrivialPoint {
  int x;
  int y;
};

consteval bool test_class_type() {
  TrivialPoint p{1, 2};
  return std::is_within_lifetime(&p);
}
static_assert(test_class_type());

consteval bool test_void_pointer() {
  int i = 0;
  return std::is_within_lifetime(static_cast<const void*>(&i));
}
static_assert(test_void_pointer());

// P3450R1: the optional first template argument U (default void): static_cast<const volatile U*>(p) must be
// well-formed.
static_assert(__cpp_lib_is_within_lifetime >= 202603L);

struct Base {
  int b = 1;
};
struct Derived : Base {
  int d = 2;
};

consteval bool test_explicit_u() {
  int i = 0;
  Derived derived;
  const Derived* pd = &derived;
  const Base* pb    = &derived;
  return std::is_within_lifetime<void>(&i) && std::is_within_lifetime<int>(&i) &&
         std::is_within_lifetime<const volatile int>(&i) && std::is_within_lifetime<Base>(pd) &&
         std::is_within_lifetime<Derived>(pd) && std::is_within_lifetime<Base>(pb) &&
         std::is_within_lifetime<Derived>(pb);
}
static_assert(test_explicit_u());

// P3450R1: false when static_cast<const volatile U*>(p) is not a constant subexpression, i.e. when the cast to a derived
// class names a type the object is not a base class subobject of, or when a void* designates an object of another type.
struct Other : Base {
  int o = 3;
};
struct Unrelated2 {
  int u = 4;
};
struct MultiDerived : Unrelated2, Base {};

consteval bool test_cast_not_constant() {
  Base base;
  Derived derived;
  Other other;
  MultiDerived multi;
  const Base* pbase        = &base;
  const Base* pderived     = &derived;
  const Base* pother       = &other;
  const Unrelated2* pun2   = &multi;
  const Base* pmulti_base  = &multi;
  int i                    = 0;
  const void* pv           = &i;
  return !std::is_within_lifetime<Derived>(pbase) && std::is_within_lifetime<Derived>(pderived) &&
         !std::is_within_lifetime<Derived>(pother) && std::is_within_lifetime<Other>(pother) &&
         std::is_within_lifetime<MultiDerived>(pun2) && std::is_within_lifetime<MultiDerived>(pmulti_base) &&
         !std::is_within_lifetime<Derived>(pmulti_base) && std::is_within_lifetime<int>(pv) &&
         !std::is_within_lifetime<char>(pv) && std::is_within_lifetime<void>(pv);
}
static_assert(test_cast_not_constant());

// A pointer to a subobject that is within its lifetime but is not a Derived: still false.
consteval bool test_cast_through_member() {
  struct Holder {
    Base b;
    Derived d;
  } holder;
  return !std::is_within_lifetime<Derived>(&holder.b) && std::is_within_lifetime<Derived>(static_cast<const Base*>(&holder.d));
}
static_assert(test_cast_through_member());

// U is the first template parameter, T is deduced
static_assert(std::is_same_v<decltype(std::is_within_lifetime<void>((const int*)nullptr)), bool>);

#endif // __cpp_lib_is_within_lifetime

int main(int, char**) { return 0; }
