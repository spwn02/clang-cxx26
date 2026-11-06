// Do expressions (P2806) survive a named module round trip, including
// init-captures (see PCH/cxx29-do-expr.cpp for the serialization trap).

// RUN: rm -rf %t && mkdir %t && split-file %s %t
// RUN: %clang_cc1 -std=c++29 -emit-module-interface %t/m.cppm -o %t/M.pcm
// RUN: %clang_cc1 -std=c++29 -fmodule-file=M=%t/M.pcm -fsyntax-only -verify %t/u.cpp
// RUN: %clang_cc1 -std=c++29 -fmodule-file=M=%t/M.pcm -emit-llvm -o - %t/u.cpp | FileCheck %s
// RUN: %clang_cc1 -std=c++29 -emit-reduced-module-interface %t/m.cppm -o %t/R.pcm
// RUN: %clang_cc1 -std=c++29 -fmodule-file=M=%t/R.pcm -fsyntax-only -verify %t/u.cpp

// CHECK: define {{.*}} @_Z3usev

//--- m.cppm
export module M;
export constexpr int sum4 = do { int s = 0; for (int i = 0; i < 4; ++i) s += i; do_return s; };
export template <class T> constexpr T twice(T v) { return do [k = v] -> T { do_return k + k; }; }
export inline int pick(bool b) { return do [q = 1] { if (b) do_return q; do_return 2; }; }
//--- u.cpp
// expected-no-diagnostics
import M;
static_assert(sum4 == 6);
static_assert(twice(21) == 42);
int use() { return pick(true) + twice(1); }
