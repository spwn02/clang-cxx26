// RUN: %clang_cc1 -std=c++23 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++2c -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions

namespace cwg2529 { // cwg2529: no
// FIXME: the destruction of the temporary bound to a constexpr reference is
// not evaluated as part of constant destruction; such a reference is rejected.
struct S {
  constexpr S() {}
  constexpr ~S() {}
};
constexpr const S &r = S();
// expected-error@-1 {{}}
// expected-note@-2 {{}}
} // namespace cwg2529
