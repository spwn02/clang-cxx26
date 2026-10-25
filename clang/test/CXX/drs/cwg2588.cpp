// RUN: rm -rf %t
// RUN: mkdir %t
// RUN: split-file %s %t
// RUN: %clang_cc1 -std=c++20 -triple x86_64-unknown-unknown -emit-module-interface %t/M.cppm -o %t/M.pcm
// RUN: %clang_cc1 -std=c++20 -triple x86_64-unknown-unknown -fprebuilt-module-path=%t -verify %t/use.cpp -fsyntax-only

// cwg2588: partial
// A function first declared by a friend declaration that is a definition in a
// class defined in an export-declaration has the linkage of the class; any
// other friend function first declared in a module has module linkage.
// FIXME: the friends 'g' and 'h' are found by an importer through
// argument-dependent lookup although they have module linkage and are not
// exported.

//--- M.cppm
export module M;
export class X {
  friend void f(X) {}
  friend void g(X);
};
class Y {
  friend void h(Y) {}
};
export Y *mk();

//--- use.cpp
// expected-no-diagnostics
import M;
void t(X x) {
  f(x);
  g(x); // FIXME: 'g' has module linkage and is not exported: ill-formed
  h(*mk()); // FIXME: likewise for 'h'
}
