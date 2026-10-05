// RUN: rm -rf %t
// RUN: mkdir -p %t
// RUN: split-file %s %t
//
// RUN: %clang_cc1 -std=c++26 -triple x86_64-linux-gnu -emit-module-interface %t/m.cppm -o %t/m.pcm
// RUN: %clang_cc1 -std=c++26 -triple x86_64-linux-gnu -fmodule-file=m=%t/m.pcm %t/use.cpp -fsyntax-only -verify
// RUN: %clang_cc1 -std=c++26 -triple x86_64-linux-gnu -fmodule-file=m=%t/m.pcm %t/use.cpp -emit-llvm -o - | FileCheck %t/use.cpp
//
// Defaulted postfix increment and decrement operators are defined in the importer (module and non-module users).

//--- m.cppm
export module m;
export struct Counter {
  int v = 0;
  constexpr Counter& operator++() { ++v; return *this; }
  constexpr Counter operator++(int) = default;
};
export struct Plain {
  int v = 0;
};
export constexpr Plain& operator++(Plain& p) { ++p.v; return p; }
export constexpr Plain operator++(Plain&, int) = default;
export template <class T> struct Wrap {
  T t{};
  constexpr Wrap& operator++() { ++t; return *this; }
  constexpr Wrap operator++(int) = default;
};

//--- use.cpp
// expected-no-diagnostics
import m;
static_assert([] { Counter c; Counter o = c++; return o.v == 0 && c.v == 1; }());
static_assert([] { Plain p; Plain o = p++; return o.v == 0 && p.v == 1; }());
static_assert([] { Wrap<int> w; Wrap<int> o = w++; return o.t == 0 && w.t == 1; }());
// CHECK-LABEL: define {{.*}} @_Z3useRW1m7Counter(
// CHECK: call {{.*}} @_ZNW1m7CounterppEi(
int use(Counter& c) { return (c++).v; }
