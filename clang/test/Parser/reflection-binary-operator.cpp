// RUN: %clang_cc1 -std=c++26 -freflection -fsyntax-only -verify=expected %s
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify=expected %s

void test() {
  int a = 1, b = 2;
  int c = a ^^ b; // expected-error 1-2 {{expected expression}} expected-error 0-1 {{type name requires a specifier or qualifier}}
}
