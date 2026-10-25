// RUN: rm -rf %t
// RUN: mkdir %t
// RUN: split-file %s %t
// RUN: %clang_cc1 -std=c++20 -triple x86_64-unknown-unknown -emit-module-interface %t/M.cppm -o %t/M.pcm
// RUN: %clang_cc1 -std=c++20 -triple x86_64-unknown-unknown -fprebuilt-module-path=%t -verify %t/use.cpp -fsyntax-only
// RUN: %clang_cc1 -std=c++20 -triple x86_64-unknown-unknown -emit-llvm %t/M.cppm -o - | FileCheck %s

// cwg2588: yes
// A function first declared by a friend declaration that is a definition in a
// class defined in an export-declaration has the linkage of the class; any
// other friend function first declared in a module has module linkage. All of
// them are found by argument-dependent lookup through the reachable class
// ([basic.lookup.argdep]p4), exported or not.

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
void g(X) {}
export void use(X x, Y y) {
  f(x);
  h(y);
}

// CHECK-DAG: define {{.*}}@_ZW1M1fS_1X(
// CHECK-DAG: define {{.*}}@_ZW1M1gS_1X(
// CHECK-DAG: define {{.*}}@_ZW1M1hS_1Y(

//--- use.cpp
// expected-no-diagnostics
import M;
void t(X x) {
  f(x);
  g(x);
  h(*mk());
}
