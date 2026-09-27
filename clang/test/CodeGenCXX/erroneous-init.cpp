// RUN: %clang --target=x86_64-linux-gnu -std=c++26 -S -emit-llvm -o - %s | FileCheck %s --check-prefixes=CHECK,PATTERN
// RUN: %clang --target=x86_64-linux-gnu -std=c++2c -S -emit-llvm -o - %s | FileCheck %s --check-prefixes=CHECK,PATTERN
// RUN: %clang --target=x86_64-linux-gnu -std=c++23 -S -emit-llvm -o - %s | FileCheck %s --check-prefixes=CHECK,NONE
// RUN: %clang --target=x86_64-linux-gnu -std=c++26 -fno-erroneous-initialization -S -emit-llvm -o - %s | FileCheck %s --check-prefixes=CHECK,NONE
// RUN: %clang --target=x86_64-linux-gnu -std=c++26 -fno-erroneous-initialization -ferroneous-initialization -S -emit-llvm -o - %s | FileCheck %s --check-prefixes=CHECK,PATTERN
// RUN: %clang --target=x86_64-linux-gnu -std=c++26 -ftrivial-auto-var-init=uninitialized -S -emit-llvm -o - %s | FileCheck %s --check-prefixes=CHECK,NONE
// RUN: %clang --target=x86_64-linux-gnu -std=c++26 -ftrivial-auto-var-init=zero -S -emit-llvm -o - %s | FileCheck %s --check-prefixes=CHECK,ZERO
// RUN: %clang --target=x86_64-linux-gnu -std=c++26 -fsanitize=memory -S -emit-llvm -o - %s | FileCheck %s --check-prefixes=CHECK,MSAN

// P2795R5: reading an uninitialized automatic variable is erroneous behavior in C++26; the value read
// is a fixed, implementation-defined pattern. `[[indeterminate]]` opts a variable out.

void use(int);
void usep(void *);
struct P { int a; char b; };

// CHECK-LABEL: define {{.*}}void @_Z6scalarv()
// PATTERN: store i32 -1431655766, ptr %x
// ZERO: store i32 0, ptr %x
// NONE-NOT: store i32
// MSAN-NOT: -1431655766
void scalar() {
  int x;
  use(x);
}

// CHECK-LABEL: define {{.*}}void @_Z5arrayv()
// PATTERN: call void @llvm.memcpy.p0.p0.i64(ptr align 16 %a, ptr align 16 @__const._Z5arrayv.a, i64 16, i1 false)
// ZERO: call void @llvm.memset.p0.i64(ptr align 16 %a, i8 0, i64 16, i1 false)
// NONE-NOT: memset
// NONE-NOT: memcpy
// MSAN-NOT: @__const
void array() {
  int a[4];
  usep(a);
}

// CHECK-LABEL: define {{.*}}void @_Z9structurev()
// PATTERN: call void @llvm.memcpy.p0.p0.i64(ptr align 4 %p, ptr align 4 @__const._Z9structurev.p,
// ZERO: call void @llvm.memset.p0.i64(ptr align 4 %p, i8 0,
// NONE-NOT: memset
// NONE-NOT: memcpy
// MSAN-NOT: @__const
void structure() {
  P p;
  usep(&p);
}

// CHECK-LABEL: define {{.*}}void @_Z5indetv()
// CHECK-NOT: store i32 -1431655766
// CHECK-NOT: @__const
void indet() {
  [[indeterminate]] int x;
  [[indeterminate]] int a[4];
  usep(&x);
  usep(a);
}

// CHECK-LABEL: define {{.*}}void @_Z11initializedv()
// CHECK-NOT: -1431655766
// CHECK: store i32 5, ptr %x
void initialized() {
  int x = 5;
  use(x);
}

// CHECK-LABEL: define {{.*}}void @_Z10uninitAttrv()
// CHECK-NOT: -1431655766
// CHECK-NOT: @__const
void uninitAttr() {
  [[clang::uninitialized]] int x;
  usep(&x);
}
