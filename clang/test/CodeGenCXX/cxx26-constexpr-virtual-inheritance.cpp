// RUN: %clang_cc1 -std=c++26 -triple x86_64-linux-gnu -emit-llvm -o - %s | FileCheck %s

// P3533R2: a constexpr variable of a class with virtual bases is emitted as a
// constant, including the virtual base subobject and the vtable pointers.

struct V {
  int n = 7;
  constexpr V() = default;
  constexpr V(int n) : n(n) {}
  constexpr virtual int f() const { return n; }
  constexpr virtual ~V() = default;
};
struct L : virtual V {
  int l = 5;
  constexpr L() : V(1) {}
};
struct R : virtual V {
  int r = 6;
  constexpr R() : V(2) {}
};
struct D : L, R {
  int d = 8;
  constexpr D() : V(3) {}
  constexpr int f() const override { return 100 + n; }
};

// CHECK: @_ZL2gd = internal constant { ptr, i32, ptr, i32, i32, ptr, i32 } { ptr {{.*}}@_ZTV1D{{.*}}, i32 5, ptr {{.*}}@_ZTV1D{{.*}}, i32 6, i32 8, ptr {{.*}}@_ZTV1D{{.*}}, i32 3 }
constexpr D gd;

// No dynamic initialization is needed.
// CHECK-NOT: __cxx_global_var_init

// CHECK-LABEL: define {{.*}}i32 @_Z3getv()
int get() { return gd.n + gd.l + gd.r + gd.d; }
