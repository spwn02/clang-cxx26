// (cc1 has exceptions disabled unless -fcxx-exceptions is given.)
// RUN: %clang_cc1 -std=c++2d -fsyntax-only -Wno-unevaluated-expression -verify=cxx29,common %s
// RUN: %clang_cc1 -std=c++2c -fsyntax-only -Wno-unevaluated-expression -verify=cxx26,common %s

// P3549: std::noreturn_t is named decltype(throw 0). A throw-expression in an unevaluated operand throws nothing, so
// -fno-exceptions does not reject it in C++29, which keeps the library usable without exceptions. A throw that can
// run is still an error.

using noreturn_t = decltype(throw 0); // cxx26-error {{cannot use 'throw' with exceptions disabled}}
bool b = noexcept(throw 1);           // cxx26-error {{cannot use 'throw' with exceptions disabled}}

void f() {
  throw 3; // common-error {{cannot use 'throw' with exceptions disabled}}
}
