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

void use_using() {
  using A::f;
  using B::f;
  f(3, 4); // OK, default argument was not used for viability
  f(3);    // expected-error {{default argument for 2nd parameter of 'f' is provided by declarations from more than one scope}}
  f();     // OK, default arguments provided by declarations in the scope of A
  void (*p)(int, int) = f;
}
void use_using_one_scope() {
  using B::f;
  f(1); // OK, only B is considered
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

// The default arguments of a declaration that overload resolution does not
// consider do not make a candidate viable.
namespace considered_scopes {
void f(int);
void f(...);
void a() { void f(int = 1); }
void b() { f(); } // OK: calls f(...), the block-scope declaration in a() is not considered

namespace CA { extern "C" void g(int = 1); }
namespace CB {
extern "C" void g(int);
void g(...);
}
void c() {
  using CB::g;
  g(); // OK: only CB's declarations are considered, calls g(...)
}
} // namespace considered_scopes

// Instantiation: the call is checked like any other.
template <class T> void tmpl() {
  using A::f;
  using B::f;
  f();
}
template void tmpl<int>();

// The scope of an instantiation is not the scope of the name: only the
// declarations found where the template names the function are considered.
namespace UA { extern "C" int uf(int, int = 5); }
namespace UB { extern "C" int uf(int, int = 7); }
using namespace UA;
using namespace UB;
template <class T> int instantiated(T t) {
  using UA::uf;
  return uf(t);
}
template int instantiated<int>(int);
int instantiated_implicitly = instantiated(1L);
