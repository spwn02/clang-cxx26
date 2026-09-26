// RUN: %clang_cc1 -std=c++26 -triple x86_64-unknown-linux-gnu -emit-llvm -ftrivial-auto-var-init=zero -o - %s | FileCheck %s

// P2795R5: [[indeterminate]] opts a variable out of automatic initialization, like [[clang::uninitialized]].

void use(int *);

// CHECK-LABEL: define {{.*}} @_Z5plainv()
// CHECK: %x = alloca i32
// CHECK: store i32 0, ptr %x
void plain() {
  int x;
  use(&x);
}

// CHECK-LABEL: define {{.*}} @_Z13indeterminatev()
// CHECK: %x = alloca i32
// CHECK-NOT: store i32 0, ptr %x
// CHECK: call void @_Z3usePi
void indeterminate() {
  int x [[indeterminate]];
  use(&x);
}

// CHECK-LABEL: define {{.*}} @_Z5arrayv()
// CHECK: %buf = alloca [256 x i8]
// CHECK-NOT: memset
// CHECK: ret void
void array() {
  char buf [[indeterminate]] [256];
  use(reinterpret_cast<int *>(buf));
}
