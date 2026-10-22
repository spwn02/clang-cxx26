// RUN: %clang_cc1 -std=c++26 -fcxx-exceptions -fexceptions -fcontract-evaluation-semantic=observe -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -std=c++26 -fcxx-exceptions -fexceptions -fcontract-evaluation-semantic=enforce -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -std=c++26 -fcxx-exceptions -fexceptions -fcontract-evaluation-semantic=quick_enforce -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -std=c++26 -fcxx-exceptions -fexceptions -fcontract-evaluation-semantic=ignore -emit-llvm -o - %s | FileCheck %s

// A function with a postcondition that cannot return normally (its body ends in a throw, an
// infinite loop or __builtin_unreachable) exits through no postcondition assertion
// ([dcl.contract.func]: a postcondition assertion is associated with exiting a function
// normally). The return value was created without an insertion point and the assertions were
// emitted after it: 'Use still stuck around after Def is destroyed' aborted the compiler for a
// non-void function.

// CHECK-LABEL: define {{.*}} @_Z6throwsv(
int throws() post(true) { throw 1; }

// CHECK-LABEL: define {{.*}} @_Z5namedv(
int named() post(r: r > 0) { throw 1; }

// CHECK-LABEL: define {{.*}} @_Z4loopv(
int loop() post(true) { for (;;) {} }

// CHECK-LABEL: define {{.*}} @_Z5unreav(
int unrea() post(true) { __builtin_unreachable(); }

// CHECK-LABEL: define {{.*}} @_Z6bothwsb(
int bothws(bool b) post(r: r == 1) { if (b) throw 1; return 1; }
