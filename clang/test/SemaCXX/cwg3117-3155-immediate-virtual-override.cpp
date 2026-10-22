// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

// [class.virtual]: a class with an immediate virtual function that overrides a
// non-immediate virtual function has consteval-only type; an immediate virtual
// function is not overridden by a non-immediate one (CWG3117, CWG3155).

struct B { virtual int f(int); }; // expected-note 3 {{overridden virtual function is here}}

struct D1 : B {
  consteval int f(int) override; // expected-error {{consteval function 'f' cannot override a non-consteval function unless its class has consteval-only type}}
};

struct D2 : B {
  decltype(^^int) i; // consteval-only member
  consteval int f(int x) override { return x; }
};

struct B3 { consteval virtual int g(); }; // expected-note {{overridden virtual function is here}}
struct D3 : B3 {
  int g() override; // expected-error {{non-consteval function 'g' cannot override a consteval function}}
};

consteval int c(int x) { return x; }

// A virtual function of a class template that is immediate-escalating becomes
// immediate when the body calls an immediate function with a non-constant argument.
template <class T> struct D4 : B {
  constexpr int f(int x) override { return c(x); } // expected-error 2 {{consteval function 'f' cannot override a non-consteval function unless its class has consteval-only type}}
};
D4<int> d4;
// expected-note@30 {{in instantiation of member function 'D4<int>::f' requested here}}
// expected-note@30 {{in instantiation of template class 'D4<int>' requested here}}
