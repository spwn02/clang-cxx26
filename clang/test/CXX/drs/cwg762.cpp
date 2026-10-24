// RUN: %clang_cc1 -std=c++98 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++11 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++14 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++17 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++20 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++23 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++2c -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// expected-note@*:* 0+ {{}}
// expected-warning@*:* 0+ {{}}

namespace cwg762 { // cwg762: yes
#if __cplusplus >= 201103L
namespace p1 {
int x;
void f() {
  int y = 0;
  auto l = [] { return x; };
  auto m = [y] { return y; };
  (void)l; (void)m;
}
}
namespace p2 {
void f() {
  int y = 0;
  auto m = [] { return y; };
}
// expected-error@-2 {{}}
}
#endif
} // namespace cwg762
