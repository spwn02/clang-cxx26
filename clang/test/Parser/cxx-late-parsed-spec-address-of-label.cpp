// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s

// The recovery from an error in the exception specification or in the
// predicate of a contract specifier of a member function, which are parsed after
// the class, read the following 'and' as '&&' of the GNU address of a label and
// declared a label in a scope that does not hold declarations: assertion
// "Scope shouldn't contain decls!" in Sema::ActOnPopScope.

template <class T> struct S {
  auto f() noexcept(std::g<T> and std::h<T>) -> S & = delete;
  // expected-error@-1 {{use of undeclared identifier 'std'}}
  // expected-error@-2 {{'T' does not refer to a value}}
  // expected-error@-3 {{expected ')'}}
  // expected-note@-4 {{to match this '('}}
  // expected-note@9 {{declared here}}
  // expected-error@-6 {{use of undeclared label 'std'}}

  void g() pre(std::g<T> and std::h<T>);
  // expected-error@-1 {{use of undeclared identifier 'std'}}
  // expected-error@-2 {{'T' does not refer to a value}}
  // expected-error@-3 {{expected ')'}}
  // expected-note@-4 {{to match this '('}}
  // expected-note@9 {{declared here}}
  // expected-error@-6 {{use of undeclared label 'std'}}
};
