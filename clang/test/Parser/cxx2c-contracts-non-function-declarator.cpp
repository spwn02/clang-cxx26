// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s

// 'pre' and 'post' after the declarator of anything but a function are not
// contract specifiers (the parser asserted that the declarator was a function).
int (x) post; // expected-error {{expected ';' after top level declarator}}
int y pre(1); // expected-error {{expected ';' after top level declarator}}
struct S {
  int z post(1); // expected-error {{expected ';' at end of declaration list}}
};

// and they still are after a function declarator
int g(const int a) post(r: r > 0) { return a + 1; }
