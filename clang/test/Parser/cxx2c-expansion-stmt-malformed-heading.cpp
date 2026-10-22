// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

// The heading of an expansion statement is a for-range-declaration and an
// expansion-initializer ([stmt.expand]). A heading that is not one is diagnosed
// and recovered from; the first of these used to assert in the parser (the
// recovery returns another statement), the others were dropped without a
// diagnostic.
void a() {
  template for (constexpr auto x(1)) { } // expected-error 2 {{expected ';' in 'for' statement specifier}}
}

void b() {
  template for (int i = 0; i < 3; ++i) { } // expected-error {{expected ':'}}
}

void c() {
  template for (int i = 0; i < 3; ++i) { static_assert(false); } // expected-error {{expected ':'}}
  int ok = 0;
  (void)ok;
}

void d() {
  template for (constexpr auto x : {1, 2}) { static_assert(x > 0); } // fine
}

// A declarator in the heading that is a function or has a qualifier: the
// context of the heading is the expansion statement, not a function body.
namespace ns {}
void e() {
  template for (int f(int) : {1, 2}) { } // expected-error {{for range declaration must declare a variable}}
}
void g() {
  template for (int ns::b : {1, 2}) { } // expected-error {{definition or redeclaration of 'b' not allowed inside a function}} expected-error {{no member named 'b' in namespace 'ns'}}
}
void h() {
  template for (int ns::b(int) : {1, 2}) { } // expected-error {{definition or redeclaration of 'b' not allowed inside a function}} expected-error {{for range declaration must declare a variable}}
}
