// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify -verify-ignore-unexpected=note %s
// [expr.const.defns] example (P2686R5): which variables are constant-initialized. constinit checks the
// static/thread-storage bullet (representable at the nearest namespace-scope point); constexpr the
// automatic one.
void f() {
  int ax = 0;
  thread_local int tx = 0;
  static int sx;
  constinit static int &rss = sx;          // OK: constant-initialized
  constinit static int &rst = tx; // expected-error {{variable does not have a constant initializer}}
  constinit static int &rsa = ax; // expected-error {{variable does not have a constant initializer}}
  constinit thread_local int &rts = sx;    // OK
  constinit thread_local int &rtt = tx; // expected-error {{variable does not have a constant initializer}}
  constinit thread_local int &rta = ax; // expected-error {{variable does not have a constant initializer}}
  constexpr int &ras = sx;                 // OK
  constexpr int &rat = tx; // expected-error {{must be initialized by a constant expression}}
  constexpr int &raa = ax;                 // OK (P2686R5)
}
