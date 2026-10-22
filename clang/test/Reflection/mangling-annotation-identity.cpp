// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -emit-llvm -o %t.direct.ll %s
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -x c++-header -emit-pch -o %t.pch %S/Inputs/mangling-annotation-identity.h
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -include-pch %t.pch -emit-llvm -o %t.pch.ll %s
// RUN: FileCheck %s < %t.direct.ll
// RUN: FileCheck %s < %t.pch.ll
// RUN: FileCheck %s --check-prefix=SYM < %t.direct.ll
// RUN: FileCheck %s --check-prefix=SYM < %t.pch.ll

// Annotation reflections of distinct annotations keep distinct specializations
// even when the annotation values are identical; compiling at all proves it,
// since equal symbols are a redefinition error.

#include "Inputs/mangling-annotation-identity.h"
using info = decltype(^^int);
struct sentinel {};
consteval info annotation(info r, unsigned i) {
  return __metafunction(105, r, ^^sentinel, i, ^^annotation);
}
consteval info parameter(info r, unsigned i = 0) {
  return __metafunction(98, r, ^^sentinel, i, ^^parameter);
}
consteval info base(info r, unsigned i = 0) {
  return __metafunction(2, r, ^^sentinel, i, ^^base);
}
template <info R> int code() { return 0; }

[[=1]] void other_function();
[[=1]] void repeated();
[[=1]] void repeated();
struct [[=1]] OtherType {};
int param([[=1]] int value);
struct BaseA {};
struct BaseB {};
struct BaseDerived : [[=1]] BaseA, [[=1]] BaseB {};
namespace [[=1]] N1 {}
namespace [[=1]] N2 {}
enum E { first [[=1]], second [[=1]] };
struct S { [[=1]] int x; [[=1]] int y; };
template <class T> struct [[=1]] W { [[=1]] void m(); [[=1]] T v; };
// Redeclarations after the annotated ones (here, after the header's) must not
// change the symbol of an earlier annotation: other translation units may see
// fewer declarations.
void header_repeated();
void header_repeated();
int param_repeated([[=1]] int value);
int param_repeated([[=1]] int value);
int param_repeated(int value);
W<int> wi;
W<char> wc;

int a = code<annotation(^^annotated_function, 0)>();
int b = code<annotation(^^other_function, 0)>();
int c = code<annotation(^^annotated_variable, 0)>();
int d = code<annotation(^^AnnotatedType, 0)>();
int e = code<annotation(^^OtherType, 0)>();
int f = code<annotation(^^repeated, 0)>();
int g = code<annotation(^^repeated, 1)>();
int h = code<annotation(parameter(^^param), 0)>();
int i = code<annotation(base(^^BaseDerived, 0), 0)>();
int j = code<annotation(base(^^BaseDerived, 1), 0)>();
int k = code<annotation(^^N1, 0)>();
int l = code<annotation(^^N2, 0)>();
int m = code<annotation(^^first, 0)>();
int n = code<annotation(^^second, 0)>();
int o = code<annotation(^^S::x, 0)>();
int p = code<annotation(^^S::y, 0)>();
int q = code<annotation(^^W<int>, 0)>();
int r = code<annotation(^^W<char>, 0)>();
int s = code<annotation(^^W<int>::m, 0)>();
int t = code<annotation(^^W<char>::m, 0)>();
int u = code<annotation(^^W<int>::v, 0)>();
int v = code<annotation(^^W<char>::v, 0)>();
int w = code<annotation(^^header_repeated, 0)>();
int x = code<annotation(^^header_repeated, 1)>();
int y = code<annotation(parameter(^^param_repeated), 0)>();
int z = code<annotation(parameter(^^param_repeated), 1)>();
// CHECK-COUNT-26: define {{.*}}@{{"?}}_Z4codeI
// SYM-DAG: define {{.*}}@_Z4codeIMaMd_Z15header_repeatedvE0_0_Li1EEEiv
// SYM-DAG: define {{.*}}@_Z4codeIMaMd_Z15header_repeatedvE1_0_Li1EEEiv
// SYM-DAG: define {{.*}}@_Z4codeIMapMd_Z14param_repeatediE0_0_0_Li1EEEiv
// SYM-DAG: define {{.*}}@_Z4codeIMapMd_Z14param_repeatediE0_1_0_Li1EEEiv
