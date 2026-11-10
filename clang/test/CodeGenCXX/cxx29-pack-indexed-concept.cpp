// RUN: %clang_cc1 -std=c++2d -triple x86_64-linux-gnu -emit-llvm -o - %s | FileCheck %s

// A pack-index-template-name as a type-constraint is resolved when the pack
// is known: the constraint is checked with the selected concept.
template <class T> concept IsInt = __is_same(T, int);
template <class T> concept Any = true;

template <template <class> concept... CC>
struct S {
  template <CC...[0] T> static int f(T) { return 1; }
  static int g(CC...[1] auto) { return 2; }
};

// The member templates keep their declared (dependent) constraint in the
// mangled name (provisional encoding, see the mangling tests).
// CHECK: define {{.*}} @_ZN1SIJ3Any5IsIntEE1fITku10pack_indexT_Li0EiEEiT_
// CHECK: define {{.*}} @_ZN1SIJ3Any5IsIntEE1gITku10pack_indexT_Li1EiEEiT_
int use() { return S<Any, IsInt>::f(1) + S<Any, IsInt>::g(1); }
