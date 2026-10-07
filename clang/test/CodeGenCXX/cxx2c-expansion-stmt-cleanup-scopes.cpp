// An expansion statement over a range leaves the scope of its range variable through 'continue', 'break' or
// 'return'. With lifetime markers (optimizations, -fsanitize-address-use-after-scope) the range variable has a
// cleanup, which used to be popped after the exit block was emitted and crashed code generation.

// RUN: %clang_cc1 -std=c++26 -fexpansion-statements -triple x86_64-unknown-linux-gnu -emit-llvm -o - %s -O1 -disable-llvm-passes | FileCheck %s
// RUN: %clang_cc1 -std=c++26 -fexpansion-statements -triple x86_64-unknown-linux-gnu -emit-llvm -o - %s -O1 | FileCheck %s
// RUN: %clang_cc1 -std=c++26 -fexpansion-statements -triple x86_64-unknown-linux-gnu -emit-llvm -o - %s -fsanitize=address -fsanitize-address-use-after-scope | FileCheck %s
// RUN: %clang_cc1 -std=c++26 -fexpansion-statements -triple x86_64-unknown-linux-gnu -emit-llvm -o - %s | FileCheck %s

struct D {
  D();
  ~D();
};

struct Range {
  const int *b, *e;
  constexpr const int *begin() const { return b; }
  constexpr const int *end() const { return e; }
};

static constexpr int arr[] = {1, 2, 3};

// CHECK-LABEL: define {{.*}} @_Z13all_continuedv(
// CHECK: call void @_ZN1DD1Ev
// CHECK: ret void
void all_continued() {
  constexpr Range r{arr, arr + 3};
  template for (constexpr int v : r) {
    D d;
    continue;
  }
}

// CHECK-LABEL: define {{.*}} @_Z6brokenv(
// CHECK: call void @_ZN1DD1Ev
// CHECK: ret i32
int broken() {
  constexpr Range r{arr, arr + 3};
  int n = 0;
  template for (constexpr int v : r) {
    D d;
    if constexpr (v == 2)
      break;
    n += v;
  }
  return n;
}

// CHECK-LABEL: define {{.*}} @_Z8returnedv(
// CHECK: call void @_ZN1DD1Ev
// CHECK: ret i32
int returned() {
  constexpr Range r{arr, arr + 3};
  template for (constexpr int v : r) {
    D d;
    if constexpr (v == 3)
      return v;
  }
  return 0;
}
