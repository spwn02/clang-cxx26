// RUN: %clang_cc1 -std=c++2d -triple x86_64-unknown-unknown -fcxx-exceptions -O0 -emit-llvm %s -o - | FileCheck %s

// P3549 with P2688: a diverging match arm stores no value; the arm ends in 'unreachable' and no poison is stored.

[[noreturn]] void die();

// CHECK-LABEL: define {{.*}}i32 @_Z1fi(
// CHECK:       call void @_Z3diev()
// CHECK-NEXT:  unreachable
// CHECK-NOT:   poison
// CHECK:       ret i32
int f(int i) { return match (i) { case 0 => 10; case _ => die(); }; }

// CHECK-LABEL: define {{.*}}i32 @_Z1gi(
// CHECK:       call void @_Z3diev()
// CHECK:       unreachable
// CHECK-NOT:   poison
// CHECK:       ret i32
int g(int i) { return match (i) { case 1 => do { die(); }; case _ => 7; }; }
