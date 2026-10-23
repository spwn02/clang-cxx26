// RUN: %clang_cc1 -std=c++20 -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s
// expected-no-diagnostics

// [temp.res.general]: the type-specifier of a conversion-type-id is a type-only context, so
// 'typename' is not needed for a dependent qualified name there (CWG2413).

template <class T> struct S {
  operator T::X();
  operator T::X *();
  operator const T::X &() const;
  operator typename T::Y();
};

struct A { using X = int; using Y = long; };
S<A> s;
