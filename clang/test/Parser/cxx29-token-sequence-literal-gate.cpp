// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

// Token sequence injection (P4380) is C++29 only: '__macro' is an ordinary
// identifier before it, and a token sequence literal is diagnosed once, as
// needing C++29, instead of asking for a header that cannot help.
int __macro = 1;
constexpr auto x = ^^{ a b { c } d }; // expected-error {{token sequence literals '^^{ ... }' are only available in C++29}}
int y = __macro;
