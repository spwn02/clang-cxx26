// RUN: %clang_cc1 -std=c++23 -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

// Code that is not evaluated at compile time still cannot use a throw-expression or a try-block when the exception
// handling of the run time is disabled, also in C++26.

void runtime() {
  try {} catch (...) {} // expected-error {{cannot use 'try' with exceptions disabled}}
  auto l = [] { throw 1; }; // expected-error {{cannot use 'throw' with exceptions disabled}}
  (void)l;
}
