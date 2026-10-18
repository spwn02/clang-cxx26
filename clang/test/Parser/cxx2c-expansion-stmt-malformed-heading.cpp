// RUN: %clang_cc1 -std=c++26 -freflection -fexpansion-statements -fsyntax-only -verify %s

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
