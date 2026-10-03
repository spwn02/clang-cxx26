// RUN: %clang_cc1 -std=c++26 -triple x86_64-unknown-linux-gnu -emit-llvm -o - %s | FileCheck %s --implicit-check-not='@{{.*}} = '
// RUN: %clang_cc1 -std=c++26 -triple x86_64-unknown-linux-gnu -fmerge-all-constants -emit-llvm -o - %s | FileCheck %s --implicit-check-not='@{{.*}} = '
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s
// expected-no-diagnostics

struct S { int *p; int &r; };
constexpr bool addresses(int n) {
  int a = n;
  constexpr int *p = &a;
  constexpr int &r = a;
  constexpr S s{&a, a};
  constexpr const int &t = 42;
  const int *tp = &t;
  int *pp = p;
  int *rp = &r;
  int *sp = s.p;
  *p += 2;
  return pp == &a && rp == &a && sp == &a && &s.r == &a &&
         r == n + 2 && *tp == 42;
}
static_assert(addresses(3));

// CHECK-LABEL: define{{.*}} @_Z4testi(
int test(int n) { return addresses(n); }
// CHECK-LABEL: define{{.*}} @_Z9addressesi(
// CHECK: %a = alloca i32
// CHECK: %p = alloca ptr
// CHECK: %r = alloca ptr
// CHECK: %s = alloca %struct.S
// CHECK: %t = alloca ptr
// CHECK: %ref.tmp = alloca i32
// CHECK: store ptr %a, ptr %p
// CHECK: store ptr %a, ptr %r
// CHECK: store i32 42, ptr %ref.tmp
// CHECK: store ptr %ref.tmp, ptr %t
// CHECK: load ptr, ptr %p
// CHECK: load ptr, ptr %r

constexpr int values(int n) {
  constexpr int a = 3;
  constexpr const int &r = a;
  constexpr const int *p = &a;
  static_assert(r == 3 && *p == 3);
  constexpr const int &t = 42;
  static_assert(t == 42);
  int arr[2] = {n, 4};
  constexpr int *q = &arr[0];
  constexpr int *const *qq = &q;
  *q += 2;
  return r + *p + t + **qq;
}
static_assert(values(1) == 51);
constexpr bool capture(int n) {
  int a = n;
  constexpr int &r = a;
  constexpr int *p = &a;
  auto l = [&] { return &r == &a && p == &a && ++r == n + 1; };
  return l();
}
static_assert(capture(4));
constexpr bool copy_capture(int n) {
  int a = n;
  constexpr int *p = &a;
  constexpr int &r = a;
  auto l = [p, r] { return *p + r; };
  ++a;
  return l() == 2 * n + 1;
}
static_assert(copy_capture(4));
int main() {
  return !test(4) || values(2) != 52 || !capture(5) || !copy_capture(6);
}
