// RUN: %clang_cc1 -std=c++26 -freflection -freflection-latest -fsyntax-only -verify %s

// [over.match.viable]: the default arguments used by a call come from the
// declarations of exactly one scope; through a splice-expression that scope is
// not a block scope (n5050 overloading.tex example).
namespace A {
extern "C" void f(int, int = 5);
extern "C" void f(int = 6, int);
}
namespace B {
extern "C" void f(int, int = 7);
}

void use() {
  [:^^A::f:](3, 4);
  [:^^A::f:](); // OK, default arguments provided by declarations in the scope of A
  [:^^A::f:](3); // expected-error {{default argument for 2nd parameter of 'f' is provided by declarations from more than one scope}}

  void g(int = 8);
  g();
  [:^^g:](); // expected-error {{default arguments of 'g' used through a splice-expression are provided by a declaration whose host scope is a block scope}}
}

void h(int = 7);
constexpr auto r = ^^h;
void poison() {
  void h(int = 8);
  h();
  [:^^h:](); // expected-error {{default argument for 1st parameter of 'h' is provided by declarations from more than one scope}}
}
void call_h() {
  [:^^h:](); // expected-error {{default argument for 1st parameter of 'h' is provided by declarations from more than one scope}}
  [:r:]();   // expected-error {{default argument for 1st parameter of 'h' is provided by declarations from more than one scope}}
}

// Redeclarations in one scope may split the default arguments.
void s(int, int = 1);
void s(int = 2, int);
void ok() { s(); s(1); [:^^s:](); }
struct S { void m(int, int = 1); static void n(int = 1); };
void S::m(int = 2, int) {}
void ok2(S x) { x.m(); S::n(); [:^^S::n:](); }
