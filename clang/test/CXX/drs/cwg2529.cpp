// RUN: %clang_cc1 -std=c++23 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++2c -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions

namespace cwg2529 { // cwg2529: yes
// A temporary to which a constexpr reference is bound shall have constant
// destruction.
struct S {
  constexpr S() {}
  constexpr ~S() {}
};
constexpr const S &r = S();

struct T {
  constexpr T() {}
  ~T() {}
};
constexpr const T &r2 = T();
// expected-error@-1 {{constexpr variable 'r2' must be initialized by a constant expression}}
// expected-note@-2 {{destruction of lifetime-extended temporary with type 'const T' used in the result of a constant expression is not a constant expression}}

void f() {
  static constexpr const S &rs = S();
}
} // namespace cwg2529
