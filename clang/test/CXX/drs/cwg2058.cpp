// RUN: %clang_cc1 -std=c++98 -triple x86_64-unknown-unknown %s -verify -Wundefined-internal -fexceptions -fcxx-exceptions -pedantic-errors
// RUN: %clang_cc1 -std=c++11 -triple x86_64-unknown-unknown %s -verify -Wundefined-internal -fexceptions -fcxx-exceptions -pedantic-errors
// RUN: %clang_cc1 -std=c++14 -triple x86_64-unknown-unknown %s -verify -Wundefined-internal -fexceptions -fcxx-exceptions -pedantic-errors
// RUN: %clang_cc1 -std=c++17 -triple x86_64-unknown-unknown %s -verify -Wundefined-internal -fexceptions -fcxx-exceptions -pedantic-errors
// RUN: %clang_cc1 -std=c++20 -triple x86_64-unknown-unknown %s -verify -Wundefined-internal -fexceptions -fcxx-exceptions -pedantic-errors
// RUN: %clang_cc1 -std=c++23 -triple x86_64-unknown-unknown %s -verify -Wundefined-internal -fexceptions -fcxx-exceptions -pedantic-errors
// RUN: %clang_cc1 -std=c++2c -triple x86_64-unknown-unknown %s -verify -Wundefined-internal -fexceptions -fcxx-exceptions -pedantic-errors

namespace cwg2058 { // cwg2058: yes
// A block-scope declaration in an unnamed namespace takes the (internal)
// linkage of the innermost enclosing namespace.
namespace {
void f() {
  extern void g();
  // expected-warning@-1 {{function 'cwg2058::(anonymous namespace)::g' has internal linkage but is not defined}}
  extern int v;
  // expected-warning@-1 {{variable 'cwg2058::(anonymous namespace)::v' has internal linkage but is not defined}}
  g(); // expected-note {{used here}}
  v = 1; // expected-note {{used here}}
}
}
void h() { f(); }
} // namespace cwg2058
