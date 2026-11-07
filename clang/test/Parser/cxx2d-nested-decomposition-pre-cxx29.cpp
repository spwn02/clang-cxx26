// RUN: %clang_cc1 -std=c++2c -fsyntax-only -verify %s

// Nested, empty-in-nested and unnamed-pack structured bindings are C++29
// (P2688R6): earlier modes do not recognize them at all, not even as an
// extension.

struct Pair { int first, second; };
struct Outer { Pair pair; int last; };

void f(Outer value) {
  auto [[x, y], z] = value; // expected-error {{expected unqualified-id}}
}
