// RUN: %clang_cc1 -std=c++20 -fsyntax-only -verify %s

// The class named by the nested-name-specifier of a conversion-function-id has
// to be complete to look up the conversion-type-id in it. When it is not, the
// nested-name-specifier is invalid and the using-declaration is dropped; this
// used to assert in Sema::BuildUsingDeclaration.
struct A; // expected-note {{forward declaration of 'A'}}
struct D {
  typedef A T;
  using T::operator int; // expected-error {{incomplete type 'A' named in nested name specifier}}
};

template <class T> struct X; // expected-note {{template is declared here}}
template <class P> struct X<P *> {
  typedef X<char> B;
  using B::operator int; // expected-error {{implicit instantiation of undefined template 'X<char>'}}
};
