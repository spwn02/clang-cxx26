// RUN: %clang_cc1 -std=c++2c -triple x86_64-linux-gnu -emit-llvm -o - %s | FileCheck %s

// [over.inc.default]: a defaulted postfix operator is 'C tmp(c); ++c; return tmp;'.

struct Counter {
  int v = 0;
  Counter& operator++() { ++v; return *this; }
  Counter operator++(int) = default;
  Counter& operator--() { --v; return *this; }
  Counter operator--(int) = default;
};

struct Wide {
  long a = 0, b = 0;
};
Wide& operator++(Wide& w) { ++w.a; return w; }
inline Wide operator++(Wide&, int) = default;

// CHECK-LABEL: define {{.*}} @_Z4testR7Counter(
// CHECK: call {{.*}} @_ZN7CounterppEi(
// CHECK: call {{.*}} @_ZN7CountermmEi(
int test(Counter& c) {
  Counter a = c++;
  Counter b = c--;
  return a.v + b.v;
}

// The copy, the prefix call and the return of the copy:
// CHECK-LABEL: define linkonce_odr {{.*}} @_ZN7CounterppEi(
// CHECK: call void @llvm.memcpy
// CHECK: call {{.*}} @_ZN7CounterppEv(
// CHECK: ret i32

// CHECK-LABEL: define linkonce_odr {{.*}} @_ZN7CountermmEi(
// CHECK: call void @llvm.memcpy
// CHECK: call {{.*}} @_ZN7CountermmEv(
// CHECK: ret i32

// CHECK-LABEL: define {{.*}} @_Z5test2R4Wide(
// CHECK: call {{.*}} @_ZppR4Widei(
long test2(Wide& w) { return (w++).a; }

// CHECK-LABEL: define linkonce_odr {{.*}} @_ZppR4Widei(
// CHECK: call void @llvm.memcpy
// CHECK: call {{.*}} @_ZppR4Wide(
