// RUN: %clang_cc1 -std=c++2d -ast-print %s | FileCheck %s

// A fully substituted pack-index-template-name prints as the selected
// template, so the printed instantiation does not mention the (out of scope)
// pack.
template <typename T> struct A { };
template <typename T> struct B { };
template <template <typename> class... TT>
int f() { return sizeof(TT...[0]<int>) + sizeof(TT...[1]<int>{}); }
int g() { return f<A, B>(); }

// CHECK: template <template <typename> class ...TT> int f() {
// CHECK-NEXT: return sizeof(TT...[0]<int>) + sizeof (TT...[1]<int>{});
// CHECK: return sizeof(A<int>) + sizeof (B<int>{});
