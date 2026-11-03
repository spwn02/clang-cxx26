// RUN: %clang_cc1 -std=c++20 -triple x86_64-unknown-linux-gnu -emit-llvm -o - %s | FileCheck %s

// P3125R6: the pointer tagging builtins lower to pointer arithmetic that keeps the provenance, a mask of the address
// and llvm.ptrmask.

// A constant tagged pointer is a constant expression that is folded: the object plus the tag as a byte offset.
alignas(16) int Global;
// CHECK: @Tagged = {{.*}}global ptr getelementptr (i8, ptr @Global, i64 5)
const void *Tagged = __builtin_pointer_tag_insert(&Global, 5, 4);

// CHECK-LABEL: define {{.*}} @_Z6insertPvmm
// CHECK: %tagged = getelementptr i8, ptr %{{.*}}, i64 %{{.*}}
// CHECK-NOT: ptrtoint
void *insert(void *p, unsigned long tag, unsigned long) {
  return __builtin_pointer_tag_insert(p, tag, 3);
}

// CHECK-LABEL: define {{.*}} @_Z3getPvm
// CHECK: %tag_mask = sub i64 {{.*}}, 1
// CHECK: [[I:%.*]] = ptrtoint ptr %{{.*}} to i64
// CHECK: %tag = and i64 [[I]], %tag_mask
unsigned long get(void *p, unsigned long bits) {
  return __builtin_pointer_tag_get(p, bits);
}

// CHECK-LABEL: define {{.*}} @_Z5stripPvm
// CHECK: %inverted_mask = xor i64 %tag_mask, -1
// CHECK: %untagged = call ptr @llvm.ptrmask.p0.i64(ptr %{{.*}}, i64 %inverted_mask)
void *strip(void *p, unsigned long bits) {
  return __builtin_pointer_tag_strip(p, bits);
}
