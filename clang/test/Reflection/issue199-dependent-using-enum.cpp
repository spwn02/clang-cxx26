// RUN: %clang_cc1 -std=c++26 -freflection -verify %s
// [enum.udecl]/1 requires a non-dependent type.
enum class E { a };
template<auto R> void f() {
  using enum [:R:]; // expected-error {{'typename [: splice :]' is not an enumerated type}}
}
void g() { using enum [:^^E:]; static_assert(a == E::a); }
