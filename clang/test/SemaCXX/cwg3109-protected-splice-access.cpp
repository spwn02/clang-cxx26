// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

// [class.protected]: the additional access check does not apply to a protected
// member designated by a splice-expression (CWG3109).

struct B { protected: int x = 1; }; // expected-note {{can only access this member on an object of type 'D'}}
struct D : B {
  int f(B& b) { return b.[:^^D::x:]; }
  int g(B& b) { return b.x; } // expected-error {{'x' is a protected member of 'B'}}
};
