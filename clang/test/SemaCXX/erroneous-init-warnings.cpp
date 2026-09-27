// RUN: %clang_cc1 -std=c++26 -fsyntax-only -Wuninitialized -Wsometimes-uninitialized -verify %s
// RUN: %clang_cc1 -std=c++23 -fsyntax-only -Wuninitialized -Wsometimes-uninitialized -verify %s

// P2795R5: reading an uninitialized automatic variable is erroneous behavior in C++26; the
// warnings that diagnose it are unchanged (they run on the AST, before any default value is chosen).

int f(bool c) {
  int x; // expected-note {{initialize the variable 'x' to silence this warning}}
  if (c) // expected-warning {{variable 'x' is used uninitialized whenever 'if' condition is false}} \
         // expected-note {{remove the 'if' if its condition is always true}}
    x = 1;
  return x; // expected-note {{uninitialized use occurs here}}
}

int g() {
  int y; // expected-note {{initialize the variable 'y' to silence this warning}}
  return y; // expected-warning {{variable 'y' is uninitialized when used here}}
}

int h() {
  int z [[indeterminate]]; // expected-note {{initialize the variable 'z' to silence this warning}}
  return z; // expected-warning {{variable 'z' is uninitialized when used here}}
}
