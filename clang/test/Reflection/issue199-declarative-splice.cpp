// RUN: %clang_cc1 -std=c++26 -freflection -verify %s
struct S { void f(); struct In; static int sm; };
void [:^^S:]::f() {} // expected-error {{splice specifier cannot be used in a declarative nested name specifier}}
struct [:^^S:]::In {}; // expected-error {{splice specifier cannot be used in a declarative nested name specifier}}
int [:^^S:]::sm = 1; // expected-error {{splice specifier cannot be used in a declarative nested name specifier}}
struct Good { void f(); struct In; static int sm; };
void Good::f() {}
struct Good::In {};
int Good::sm = 1;
