// A tagged pointer (P3125R6) in a constant survives a named module round trip, full and reduced BMI.

// RUN: rm -rf %t && mkdir %t && split-file %s %t
// RUN: %clang_cc1 -std=c++20 -emit-module-interface %t/m.cppm -o %t/M.pcm
// RUN: %clang_cc1 -std=c++20 -fmodule-file=M=%t/M.pcm -fsyntax-only -verify %t/u.cpp
// RUN: %clang_cc1 -std=c++20 -fmodule-file=M=%t/M.pcm -emit-llvm -o - %t/u.cpp | FileCheck %t/u.cpp
// RUN: %clang_cc1 -std=c++20 -emit-reduced-module-interface %t/m.cppm -o %t/R.pcm
// RUN: %clang_cc1 -std=c++20 -fmodule-file=M=%t/R.pcm -fsyntax-only -verify %t/u.cpp

//--- m.cppm
export module M;
export alignas(8) inline constexpr int G = 1;
export constexpr const void *Tagged = __builtin_pointer_tag_insert(&G, 5, 3);
export struct Holder {
  const void *p;
};
export constexpr Holder H{__builtin_pointer_tag_insert(&G, 6, 3)};
//--- u.cpp
// expected-no-diagnostics
import M;
static_assert(__builtin_pointer_tag_get(Tagged, 3) == 5);
static_assert(__builtin_pointer_tag_strip(Tagged, 3) == &G);
static_assert(__builtin_pointer_tag_get(H.p, 3) == 6);
static_assert(Tagged != H.p);
const void *Use = H.p;
// CHECK: @Use = {{.*}}global ptr getelementptr (i8, ptr @_ZW1M1G, i64 6)
