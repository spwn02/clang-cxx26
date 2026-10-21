// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

// [temp.deduct.guide]: a generic parameter type placeholder shall not appear
// in the parameter-declaration-clause of a deduction guide.
template <class T> struct X { X(T); };
X(auto) -> X<int>;      // expected-error {{a generic parameter type placeholder cannot appear}}
X(auto *) -> X<int>;    // expected-error {{a generic parameter type placeholder cannot appear}}
X(const auto &) -> X<int>; // expected-error {{a generic parameter type placeholder cannot appear}}
template <class T> X(T, int) -> X<T>;
X(int, long) -> X<long>;
