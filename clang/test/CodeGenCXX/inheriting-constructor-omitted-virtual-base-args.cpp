// RUN: %clang_cc1 -std=c++26 -triple x86_64-linux-gnu -emit-llvm -o - %s | FileCheck %s --implicit-check-not='call {{.*}}@_Z3barv'

// [class.inhctor.init]: the invocation of the inherited constructor, including
// the evaluation of any arguments, is omitted if the virtual base subobject is
// not initialized as part of this object.

int calls = 0;
struct V { V() = default; V(int) {} };
struct Q { Q() {} };
struct A : virtual V, Q { using V::V; A() = delete; };
int bar() { ++calls; return 42; }
struct B : A { B() : A(bar()) {} };
struct C : B {};

// Neither the base-object nor the complete-object constructor of B calls bar:
// the inherited constructor of the virtual base V is not invoked by A here.
// CHECK: define {{.*}} @_ZN1BC1Ev(
// CHECK: define {{.*}} @_ZN1BC2Ev(

void f() { C c; B b; }
