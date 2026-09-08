// RUN: %clang_cc1 -verify %s

// Regression test for overload-resolution identity conversion of _Atomic(T).

// expected-no-diagnostics

void f(double);

template <class = int>
void f(double);

_Atomic double atomic_value = 42.5;

void test() {
  f(atomic_value);
}
