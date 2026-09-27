// RUN: %clang_cc1 -std=c++26 -triple x86_64-linux-gnu -emit-llvm -disable-llvm-passes -o - %s | FileCheck %s

// P3074R7: a union with a non-trivial variant member has a trivial default
// constructor and destructor, so nothing is emitted to construct or destroy it.

struct S { S(); ~S(); int x; };
union U { S s; int i; };
struct Anon { union { S s; int i; }; };
template <class T> union W { T t; int i; };

void take(void *);

// CHECK-LABEL: define {{.*}}void @_Z5localv()
// CHECK-NOT: call
// CHECK: call void @_Z4takePv
// CHECK-NOT: call
// CHECK: ret void
void local() {
  U u;
  take(&u);
}

// CHECK-LABEL: define {{.*}}void @_Z9localAnonv()
// CHECK-NOT: _ZN1SC
// CHECK-NOT: _ZN1SD
// CHECK: ret void
void localAnon() {
  Anon a;
  W<S> w;
  take(&a);
  take(&w);
}

// CHECK-LABEL: define {{.*}}void @_Z6heap26v()
// CHECK-NOT: _ZN1SD
// CHECK: ret void
void heap26() {
  U *p = new U;
  delete p;
}

// A global needs no dynamic initializer or destructor.
// CHECK-NOT: __cxa_atexit
// CHECK-NOT: _GLOBAL__sub_I
U g;
