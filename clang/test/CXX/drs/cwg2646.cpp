// RUN: %clang_cc1 -std=c++23 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++2c -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// expected-no-diagnostics

namespace cwg2646 { // cwg2646: yes
struct C {
  C();
  C &operator=(this C &, const C &) = default;
  C &operator=(this C &, C &&) = default;
};
void f(C a, C b) {
  a = b;
  a = static_cast<C &&>(b);
}
} // namespace cwg2646
