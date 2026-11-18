// RUN: %clang_cc1 -std=c++2d -fsyntax-only -verify=cxx29,both %s
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify=cxx26,both %s

// #512. DIRECT members (settled by the wording): the order of designators does
// not take part in forming an implicit conversion sequence. [over.ics.list]/2
// only asks whether the parameter "can be initialized from the initializer list
// according to the rules for aggregate initialization"; the order rule is
// [dcl.init.list]/3.1 ("non-decreasing element order"), and Note 1 under
// [over.ics.list]/2 says a mismatching order makes the initialization of the
// SELECTED parameter ill-formed. So both candidates are viable and the call is
// ambiguous (the standard's Example 1 in that subclause relies on the same rule).
struct X { int x, y; };
struct Y { int y, x; };
void d(X); // both-note {{candidate function}}
void d(Y); // both-note {{candidate function}}
void direct() {
  d({.y = 1, .x = 2}); // both-error {{call to 'd' is ambiguous}}
}

// Designators naming members of a BASE class (P2287R6). With P2287R6 the
// designators .y and .x name members of the base A, so f(B) is a candidate; before
// P2287R6 they do not name members of B, so only f(C) is viable. Here both
// designators name the same base, so the top-level order check of
// [dcl.init.list]/3.1 sees one element and the violation appears only in the
// nested "in order" initialization of the base ([dcl.init.aggr]/5.2.2): the
// wording does not say whether that affects viability. The fork treats it like
// the direct case (the candidate is viable, the error is reported when it is
// selected), so the call is ambiguous with P2287R6. This is a fork choice.
struct A { int x, y; };
struct B : A { int q; };
struct C { int y, x; };
void f(B); // cxx29-note {{candidate function}}
void f(C); // cxx29-note {{candidate function}}
void base() {
  f({.y = 1, .x = 2}); // cxx29-error {{call to 'f' is ambiguous}}
}
