// RUN: %clang_cc1 -std=c++17 -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++20 -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++23 -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

struct A {
  void not_inline();
  inline void already_inline();
};

struct B {
  friend inline void A::not_inline(); // expected-error {{friend declaration of 'not_inline' with 'inline' specifier must be a definition unless previously declared inline}}
  friend inline void A::already_inline();
};
