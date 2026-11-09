// RUN: %clang_cc1 -std=c++2d -triple x86_64-linux-gnu -emit-llvm -o - %s | FileCheck %s

struct A { int a1, a2; };
struct B : A { int b; };
struct S { int v = 7; int w = 1; };
struct D2 : A, S { int d; };

// CHECK-LABEL: define {{.*}}@_Z2f1v
// CHECK: store i32 1, ptr %{{.*}}
// CHECK: store i32 0, ptr %{{.*}}
// CHECK: store i32 2, ptr %{{.*}}
B f1() { return B{.a1 = 1, .b = 2}; }

// CHECK-LABEL: define {{.*}}@_Z2f2v
// The second base is initialized from its default member initializers.
// CHECK: store i32 4, ptr %{{.*}}
// CHECK: store i32 7, ptr %{{.*}}
// CHECK: store i32 1, ptr %{{.*}}
D2 f2() { return D2{.a2 = 4, .d = 5}; }
