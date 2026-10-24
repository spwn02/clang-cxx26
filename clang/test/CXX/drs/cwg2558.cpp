// RUN: %clang_cc1 -std=c++20 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++23 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++2c -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// expected-note@*:* 0+ {{}}

namespace cwg2558 { // cwg2558: yes
struct S {
  int a;
  int b;
  constexpr S() { a = 1; }
};
struct T {
  int a;
  int b;
  constexpr T() : a(1), b(2) {}
};
consteval S f() { return S(); }
consteval T g() { return T(); }
void h() {
  (void)f();
  // expected-error@-1 {{call to consteval function 'cwg2558::f' is not a constant expression}}
  (void)g();
}
} // namespace cwg2558
