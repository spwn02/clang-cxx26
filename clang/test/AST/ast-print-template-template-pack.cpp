// RUN: %clang_cc1 -std=c++20 -ast-print %s > %t.cpp
// RUN: %clang_cc1 -std=c++20 -fsyntax-only %t.cpp
// RUN: %clang_cc1 -std=c++20 -ast-print %s | FileCheck %s

template <typename> struct A {};
template <typename> struct B {};

template <template <typename> class... TT> int f() {
  return 0;
}
int g() { return f<A, A>(); }
int empty() { return f<>(); }

template <template <typename> class... TT> struct S {};
S<A, B> s;

template <typename... Ts> struct TypePack {};
TypePack<int, long> types;
template <int... Ns> struct ValuePack {};
ValuePack<1, 2> values;

// CHECK: template<> int f<A, A>()
// CHECK: template<> int f<>()
// CHECK: template<> struct S<A, B>
// CHECK: TypePack<int, long> types
// CHECK: ValuePack<1, 2> values
