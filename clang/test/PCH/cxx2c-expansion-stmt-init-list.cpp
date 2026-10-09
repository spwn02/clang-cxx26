// RUN: %clang_cc1 -std=c++26 -fexpansion-statements -emit-pch -o %t.pch %s
// RUN: %clang_cc1 -std=c++26 -fexpansion-statements -include-pch %t.pch -emit-llvm -o - %s | FileCheck %s

// The element list of an expansion over a braced list is read back from a PCH: whether it contains a pack is not
// serialized and has to be recomputed, otherwise the statement looks like it has a dependent size in CodeGen.

#ifndef HEADER
#define HEADER

void sink(int);

inline void f() {
  template for (int i : {1, 2, 3}) {
    sink(i);
  }
}

#else

void g() { f(); }

// CHECK-LABEL: define linkonce_odr void @_Z1fv(
// CHECK-COUNT-3: call void @_Z4sinki(i32 noundef %{{[0-9]+}})

#endif
