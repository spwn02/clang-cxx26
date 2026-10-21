// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

// [namespace.udecl]: if a declaration is named by two using-declarators that
// inhabit the same class scope, the program is ill-formed.
struct C {
  int i;
  void f(int);
};
struct D1 : C {};
struct D2 : C {};

struct D3 : D1, D2 {
  using D1::i;
  using D1::i; // expected-error {{redeclaration of using declaration}}
  using D2::i; // expected-error {{redeclaration of using declaration}}
  // expected-note@-3 2{{previous using declaration}}
};

struct D4 : C {
  using C::f;
  using C::i;
};
struct D5 : D1 {
  using D1::f;
  using D1::f; // expected-error {{redeclaration of using declaration}}
  // expected-note@-2 {{previous using declaration}}
};
