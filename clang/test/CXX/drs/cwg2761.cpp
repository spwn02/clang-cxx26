// RUN: %clang_cc1 -std=c++98 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++11 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++14 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++17 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++20 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++23 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++2c -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// expected-note@*:* 0+ {{}}
// expected-warning@*:* 0+ {{}}

namespace cwg2761 { // cwg2761: yes
#if __cplusplus >= 201103L
namespace p1 {
struct A { A(); ~A() = delete; };
struct S { union { A a; int i; }; S() : i(0) {} ~S() {} };
}
namespace p2 {
struct A { A(); ~A() = delete; };
struct S { union { A a; int i; }; S() : i(0) {} };
void f(S* p) { delete p; }
// expected-error@-1 {{}}
}
#endif
} // namespace cwg2761
