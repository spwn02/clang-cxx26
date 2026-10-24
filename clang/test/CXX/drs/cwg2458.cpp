// RUN: %clang_cc1 -std=c++98 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++11 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++14 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++17 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++20 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++23 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++2c -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// expected-note@*:* 0+ {{}}
// expected-warning@*:* 0+ {{}}

namespace cwg2458 { // cwg2458: yes
#if __cplusplus >= 201103L
namespace p1 {
struct A{void f();}; void g(A a){ auto b = a.f; }
// expected-error@-1 2 {{}}
// expected-note@*:* 0+ {{}}
}
namespace p2 {
struct A{void f();}; void g(A a){ auto b = &a.f; }
// expected-error@-1 {{}}
// expected-note@*:* 0+ {{}}
}
#endif
} // namespace cwg2458
