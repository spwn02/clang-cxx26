// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

// The contracts of a redeclaration that were malformed (already diagnosed) are
// not compared with the ones of the first declaration: this asserted
// ("!NewDecl->getContracts()->isInvalidDecl()").
int f(int x);
int f(int x) post(r: r >) { return x; } // expected-error {{expected expression}}
int g(ont;int g(ont)post{ // expected-error 2 {{use of undeclared identifier 'ont'}} expected-error {{expected ')'}} expected-note {{to match this '('}} expected-error {{expected '(' after 'post'}} expected-error {{expected '}'}} expected-note {{to match this '{'}}
