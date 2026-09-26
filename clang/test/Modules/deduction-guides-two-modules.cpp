// The implicit deduction guides of a class template that two modules each
// synthesize (both include the same header in their global module fragment) are
// not merged when the modules are imported together; deduction must not treat
// them as an ambiguity (spwn02/clang-cxx26#123).
//
// RUN: rm -rf %t
// RUN: mkdir -p %t
// RUN: split-file %s %t
//
// RUN: %clang_cc1 -std=c++20 -triple x86_64-linux-gnu -I%t -emit-reduced-module-interface %t/p1.cppm -o %t/p1.pcm
// RUN: %clang_cc1 -std=c++20 -triple x86_64-linux-gnu -I%t -emit-reduced-module-interface %t/p2.cppm -o %t/p2.pcm
// RUN: %clang_cc1 -std=c++20 -triple x86_64-linux-gnu -fsyntax-only -verify \
// RUN:   -fmodule-file=p1=%t/p1.pcm -fmodule-file=p2=%t/p2.pcm %t/use.cpp
// RUN: %clang_cc1 -std=c++20 -triple x86_64-linux-gnu -fsyntax-only -verify \
// RUN:   -fmodule-file=p2=%t/p2.pcm -fmodule-file=p1=%t/p1.pcm %t/use.cpp

//--- b.h
template <typename T> struct B {
  B(T) {}
};

//--- p1.cppm
module;
#include "b.h"
export module p1;
export using ::B;
export inline auto g1() { B b(1); B b2 = b; return sizeof(b2); }

//--- p2.cppm
module;
#include "b.h"
export module p2;
export using ::B;
export inline auto g2() { B b(1.5); return sizeof(b); }

//--- use.cpp
// expected-no-diagnostics
import p1;
import p2;

B b3(2.0);
static_assert(__is_same(decltype(b3), B<double>));
B b4(b3);
static_assert(__is_same(decltype(b4), B<double>));
B b5{3};
static_assert(__is_same(decltype(b5), B<int>));
