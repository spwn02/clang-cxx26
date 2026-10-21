// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

// [over.match.viable]: a dependent call is checked at instantiation, where
// the declarations the lookup kept at the definition are the ones considered.
namespace A { extern "C" void f(int, int = 5); extern "C" void f(int = 6, int); }
namespace B { extern "C" void f(int, int = 7); }

template<class T> void u(T x) { using A::f; using B::f; f(x); } // expected-error {{default argument for 2nd parameter of 'f' is provided by declarations from more than one scope}}
template void u<int>(int); // expected-note {{in instantiation of function template specialization 'u<int>' requested here}}

// f() takes its defaults from A only: valid.
template<class T> void t() { using A::f; using B::f; f(); }
template void t<int>();

// One scope only: valid.
template<class T> void v(T x) { using A::f; f(x); }
template void v<int>(int);
