// RUN: %clang_cc1 -std=c++2d -fsyntax-only -verify=cxx29 %s
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify=cxx26 %s -DONLY_CXX26
// cxx26-no-diagnostics

// #512: the order of designators does not take part in forming an implicit
// conversion sequence. [over.ics.list]/2 only asks whether the parameter type
// "can be initialized from the initializer list according to the rules for
// aggregate initialization", and the order rule is [dcl.init.list]/3.1
// ("non-decreasing element order"); Note 1 under [over.ics.list]/2 says a
// mismatching order makes the initialization of the SELECTED parameter
// ill-formed. So with P2287R6 (designators naming base class members) both
// candidates below are viable and the call is ambiguous; before P2287R6 the
// designator .y does not name a member of B, so only f(C) is viable.
struct A { int x, y; };
struct B : A { int q; };
struct C { int y, x; };
void f(B); // cxx29-note {{candidate function}}
void f(C); // cxx29-note {{candidate function}}
void g() {
  f({.y = 1, .x = 2}); // cxx29-error {{call to 'f' is ambiguous}}
}
