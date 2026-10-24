// RUN: %clang_cc1 -std=c++98 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++11 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++14 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++17 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++20 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++23 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++2c -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// expected-note@*:* 0+ {{}}
// expected-warning@*:* 0+ {{}}

namespace cwg1368 { // cwg1368: yes
#if __cplusplus >= 201103L
namespace p1 {
struct A { A() = default; int i; };
struct B { B(); int i; };
int f() { A a{}; B b{}; return A().i; }
}
#endif
} // namespace cwg1368
