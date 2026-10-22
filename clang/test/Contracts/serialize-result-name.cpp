// RUN: rm -rf %t
// RUN: split-file %s %t
//
// A result name is serialized with its type and its deduced-return-type
// placeholder state: a function template with a postcondition result name
// instantiated from a precompiled header (it used to assert on the null type of
// the deserialized result name), and the same through a module interface.
// RUN: %clang_cc1 -std=c++26 -x c++-header %t/h.h -emit-pch -o %t/h.pch
// RUN: %clang_cc1 -std=c++26 -include-pch %t/h.pch %t/use.cpp -emit-llvm -o - | FileCheck %s
// RUN: %clang_cc1 -std=c++26 %t/M.cppm -emit-module-interface -o %t/M.pcm
// RUN: %clang_cc1 -std=c++26 -fmodule-file=M=%t/M.pcm %t/use-module.cpp -emit-llvm -o - | FileCheck %s

// CHECK-DAG: define {{.*}} @{{.*}}1gIiE
// CHECK-DAG: define {{.*}} @{{.*}}1hv
// CHECK-DAG: define {{.*}} @{{.*}}1SIiE1mE

//--- h.h
template <class T> T g(T x) post(r: r > 0) { return x; }
inline auto h() post(r: r == 3) { return 3; }
template <class T> struct S { T m(const T x) post(r: r == x) { return x; } };
//--- use.cpp
int use() { return g(1) + h() + S<int>{}.m(2); }
//--- M.cppm
export module M;
export template <class T> T g(T x) post(r: r > 0) { return x; }
export inline auto h() post(r: r == 3) { return 3; }
export template <class T> struct S { T m(const T x) post(r: r == x) { return x; } };
//--- use-module.cpp
import M;
int use() { return g(1) + h() + S<int>{}.m(2); }
