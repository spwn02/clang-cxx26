// RUN: %clang_cc1 -std=c++2c -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++20 -fsyntax-only -verify %s

// P2287R6 is C++29 only: a designator names a direct member of the class.

struct A { int a1, a2; };
struct B : A { int b; };
B v1 = B{.a1=1, .b=2}; // expected-error {{field designator 'a1' does not refer to any field in type 'B'}}
B v2 = B{A{1, 2}, .b=3}; // expected-warning {{mixture of designated and non-designated initializers in the same initializer list is a C99 extension}} \
                         // expected-note {{first non-designated initializer is here}}
