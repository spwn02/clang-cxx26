// RUN: %clang_cc1 -std=c++2d -triple x86_64-linux-gnu -emit-llvm -o - %s | FileCheck %s

template <class> concept C = true;
template <class> concept D = true;

// A concept template parameter is named in the constraint encoding. The
// enclosing class template arguments distinguish these two specializations.
template <template <class> concept C> struct S {
  template <C T> static int f(T) { return 1; }
};

// CHECK: define {{.*}} @_ZN1SI1CE1fITkN1S1CEiEEiT_
// CHECK: define {{.*}} @_ZN1SI1DE1fITkN1S1CEiEEiT_
template int S<C>::f(int);
template int S<D>::f(int);

// A pack-indexed concept parameter uses the provisional pack-index encoding.
template <template <class> concept... CC> struct P {
  template <CC...[0] T> static int f(T) { return 1; }
};

// CHECK: define {{.*}} @_ZN1PIJ1C1DEE1fITku10pack_indexT_Li0EiEEiT_
// CHECK: define {{.*}} @_ZN1PIJ1D1CEE1fITku10pack_indexT_Li0EiEEiT_
template int P<C, D>::f(int);
template int P<D, C>::f(int);
