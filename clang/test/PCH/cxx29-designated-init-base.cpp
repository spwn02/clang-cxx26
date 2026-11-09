// RUN: %clang_cc1 -std=c++2d -x c++-header %s -emit-pch -o %t.pch
// RUN: %clang_cc1 -std=c++2d -x c++ /dev/null -include-pch %t.pch -fsyntax-only
// RUN: %clang_cc1 -std=c++2d -x c++-header %s -emit-pch -fpch-instantiate-templates -o %t.pch
// RUN: %clang_cc1 -std=c++2d -x c++ /dev/null -include-pch %t.pch -fsyntax-only

struct A { int a1, a2; };
struct B : A { int b; };
constexpr B vb = B{.a2 = 5, .b = 6};
static_assert(vb.a1 == 0 && vb.a2 == 5 && vb.b == 6);

template <class T> struct Base { T v; };
template <class T> struct Der : Base<T> { T w; };
template <class T> constexpr Der<T> mk(T t) { return Der<T>{.v = t, .w = t + 1}; }
static_assert(mk(3).v == 3 && mk(3).w == 4);
