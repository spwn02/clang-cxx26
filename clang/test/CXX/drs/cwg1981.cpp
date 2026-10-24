// RUN: %clang_cc1 -std=c++98 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++11 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++14 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++17 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++20 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++23 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++2c -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// expected-note@*:* 0+ {{}}
// expected-warning@*:* 0+ {{}}

namespace cwg1981 { // cwg1981: yes
#if __cplusplus >= 201103L
namespace p1 {
struct A { explicit operator bool() const; };
void f(A a){ if (a) {} while (a) {} (void)(a && a); (void)!a; (void)(a ? 1 : 2); for (;a;) {} }
}
namespace p2 {
struct A { explicit operator bool() const; };
void f(A a){ bool b = a; }
// expected-error@-1 {{}}
}
#endif
} // namespace cwg1981
