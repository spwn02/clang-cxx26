// RUN: %clang_cc1 -std=c++26 -freflection -verify %s
struct S { ~S(); };
void f(S* p) {
  p->[:^^S::~S:](); // expected-error {{reflection not usable in a splice expression}}
  p->~typename[:^^S:]();
}
template<auto R> void g(S* p) {
  p->[:R:](); // expected-error {{reflection not usable in a splice expression}}
}
void h(S* p) { g<^^S::~S>(p); } // expected-note {{in instantiation}}
