// RUN: %clang_cc1 -std=c++2d -triple x86_64-linux-gnu -emit-llvm -o - %s | FileCheck %s
// Provisional Itanium encoding (no ABI exists yet): Dy <template-name> <index>.
// (the signature keeps the dependent form: Dy <template-param> <index>)

template <typename T> struct A { };
template <typename T> struct B { };

template <template <typename> class... TT>
void g(TT...[1]<int> x) { }

// CHECK: define {{.*}} @_Z1gIJ1A1BEEvNDyT_Li1EIiEE
template void g<A, B>(B<int>);

// A dependent one inside a function template signature (decltype).
template <int I, template <typename> class... TT>
void h(decltype(TT...[I]<int>{})) { }

// CHECK: define {{.*}} @_Z1hILi1EJ1A1BEEvDTtlNDyT0_T_IiEEEE
template void h<1, A, B>(B<int>);
