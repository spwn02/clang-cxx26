// RUN: %clang_cc1 -std=c++17 -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

// [expr.ass]: the meaning of x = {v} is x = t, where t is an invented temporary declared and
// initialized as T t = {v}, that is copy-list-initialization (CWG2768).

enum class E : int { a };
enum U { u };

void f() {
  E e = E::a;
  e = E{0};
  e = {0}; // expected-error {{}}
  e = {};  // value-initialization: no conversion needed
  int i = 0;
  i = {1};
  i = {};
  U x = u;
  x = {u};
  int *p;
  p = {};
}
