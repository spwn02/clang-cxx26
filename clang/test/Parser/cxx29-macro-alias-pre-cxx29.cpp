// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s
// expected-no-diagnostics

// Before C++29 `macro` is an ordinary identifier, also where it would start
// an expression-macro declaration there.
struct macro {};
macro make(int);
int use() { int macro = 1; return macro; }
