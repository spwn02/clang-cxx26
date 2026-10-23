// RUN: %clang_cc1 -std=c++20 -fsyntax-only -verify %s
// expected-no-diagnostics

// [dcl.link]: a C language linkage is ignored in determining the language linkage of
// friend functions with a trailing requires-clause (CWG2460).

template <typename T> struct A { struct B; };
extern "C" {
template <typename T>
struct A<T>::B {
  friend void f(B *) requires true {}
};
}
namespace Q { extern "C" void f(); }
