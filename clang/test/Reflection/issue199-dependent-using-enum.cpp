// RUN: %clang_cc1 -std=c++26 -freflection -verify %s
// [enum.udecl]/1 requires a non-dependent type.
enum class E { a };
template<auto R> void f() {
  using enum [:R:]; // expected-error {{using-enum cannot name a dependent type}}
}
void g() { using enum [:^^E:]; static_assert(a == E::a); }
