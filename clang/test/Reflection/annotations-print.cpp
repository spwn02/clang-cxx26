// RUN: %clang_cc1 %s -std=c++26 -freflection -fannotation-attributes -verify
// RUN: %clang_cc1 %s -std=c++26 -freflection -fannotation-attributes -ast-print | FileCheck %s
// RUN: %clang_cc1 %s -std=c++26 -freflection -fannotation-attributes -ast-dump | FileCheck %s --check-prefix=DUMP
// expected-no-diagnostics
struct [[=1,=2]] S { [[=3]] int m; };
[[=4]] void f([[=5]] int x);
enum E { e [[=6]] };
namespace [[=7]] N {}
template<class T> [[=8]] void tf(T);
[[=9]] int gv = 0;
using A [[=10]] = int;
template<class T> using AT [[=11]] = T;
template<class T> [[=12]] T vt = T();
template<class T> struct SP {};
template<> struct [[=13]] SP<int> {};
auto lam = []([[=14]] int y, [[=15]] auto z) {};
// CHECK: struct {{.*}}{{\[\[=1\]\]}} {{\[\[=2\]\]}} S
// CHECK: int m {{\[\[=3\]\]}};
// CHECK: {{\[\[=4\]\]}} void f({{\[\[=5\]\]}} int x);
// CHECK: e {{\[\[=6\]\]}}
// CHECK: namespace {{\[\[=7\]\]}} N
// CHECK: template <class T> {{\[\[=8\]\]}} void tf(T);
// CHECK: {{\[\[=9\]\]}} int gv
// CHECK: using A {{\[\[=10\]\]}} = int;
// CHECK: template <class T> using AT {{\[\[=11\]\]}} = T;
// CHECK: template <class T> {{\[\[=12\]\]}} T vt
// CHECK: template<> struct {{\[\[=13\]\]}} SP<int>
// CHECK: auto lam = []({{\[\[=14\]\]}} int y, {{\[\[=15\]\]}} auto z)
// DUMP-COUNT-8: CXX26AnnotationAttr
