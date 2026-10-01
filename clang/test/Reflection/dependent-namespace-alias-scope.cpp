//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// A namespace alias to a dependent splice, used as a nested-name-specifier in
// a template (GitHub #197), must not crash and must work after instantiation.
//
// RUN: %clang_cc1 %s -std=c++23 -freflection -verify -emit-llvm -o /dev/null

// expected-no-diagnostics

using info = decltype(^^int);

template <typename T, typename U> constexpr bool same = false;
template <typename T> constexpr bool same<T, T> = true;

namespace N {
struct P {
  using t = int;
  template <typename> struct TT {
    using type = char;
    static constexpr int s = 1;
  };
};
template <typename> struct TT {
  using type = char;
  static constexpr int s = 2;
};
constexpr int nv = 7;
struct Q { using t = long; };
namespace M { constexpr int v = 3; }
} // namespace N

template <info R> constexpr int types() {
  namespace A = [:R:];
  namespace B = A; // alias of a dependent alias
  typename A::P::t u = 0;
  typename B::template TT<int> t1{};
  typename A::template TT<int>::type t2 = 0;
  typename A::P::template TT<int>::type t3 = 0;
  static_assert(same<typename A::P::t, int>);
  static_assert(same<typename B::template TT<int>::type, char>);
  static_assert(same<typename A::Q::t, long>);
  return sizeof(u) + sizeof(t1) + sizeof(t2) + sizeof(t3);
}
static_assert(types<^^N>() == 4 + 1 + 1 + 1);

template <info R> constexpr int values() {
  namespace A = [:R:];
  namespace B = A;
  static_assert(A::nv == 7);
  static_assert(B::nv == 7);
  return A::template TT<int>::s + B::nv + A::P::template TT<int>::s;
}
static_assert(values<^^N>() == 2 + 7 + 1);

// Alias to a qualified name whose qualifier is a dependent splice.
template <info R> constexpr int qualified() {
  namespace A = [:R:]::M;
  namespace B = A;
  return A::v + B::v;
}
static_assert(qualified<^^N>() == 6);

// Non-dependent aliases keep working.
namespace NA = [:^^N:];
static_assert(NA::nv == 7);
typename NA::P::t g = 1;

int use() { return types<^^N>() + values<^^N>() + qualified<^^N>(); }
