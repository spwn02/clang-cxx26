// RUN: %clang_cc1 -std=c++26 -triple x86_64-unknown-linux-gnu -emit-llvm -o - %s | FileCheck %s

// [cstdarg.syn]: va_start takes one argument in C++26 and discards the others.
#include <stdarg.h>

// CHECK-LABEL: define {{.*}}i32 @_Z3sumiz(
// CHECK: call void @llvm.va_start
// CHECK: call void @llvm.va_end
int sum(int n, ...) {
  va_list ap;
  va_start(ap);
  int total = 0;
  for (int i = 0; i < n; ++i)
    total += va_arg(ap, int);
  va_end(ap);
  return total;
}

// A function without named parameters.
// CHECK-LABEL: define {{.*}}i32 @_Z5firstz(
// CHECK: call void @llvm.va_start
int first(...) {
  va_list ap;
  va_start(ap);
  int v = va_arg(ap, int);
  va_end(ap);
  return v;
}

// The second argument is discarded, whatever it is.
// CHECK-LABEL: define {{.*}}i32 @_Z9discardediz(
// CHECK: call void @llvm.va_start
int discarded(int n, ...) {
  va_list ap;
  va_start(ap, n, not even tokens of an expression);
  int v = va_arg(ap, int);
  va_end(ap);
  return v;
}
