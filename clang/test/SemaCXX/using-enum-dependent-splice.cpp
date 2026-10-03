// RUN: %clang_cc1 -std=c++26 -freflection -fsyntax-only -verify %s

// [enum.udecl]: the using-enum-declarator shall designate a non-dependent type,
// so a dependent splice-type-specifier is rejected with the dependent-type
// diagnostic rather than "is not an enumerated type".
enum class E { a };

template <auto R> void f() {
  using enum [:R:]; // expected-error {{using-enum cannot name a dependent type}}
}

void g() {
  using enum [:^^E:]; // OK: non-dependent
  (void)a;
}
