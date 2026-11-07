// Pattern matching (P2688) survives a named module round trip: constant
// evaluation and instantiation of deserialized patterns.

// RUN: rm -rf %t && mkdir %t && split-file %s %t
// RUN: %clang_cc1 -std=c++2d -emit-module-interface %t/m.cppm -o %t/M.pcm
// RUN: %clang_cc1 -std=c++2d -fmodule-file=M=%t/M.pcm -fsyntax-only -verify %t/u.cpp
// RUN: %clang_cc1 -std=c++2d -fmodule-file=M=%t/M.pcm -emit-llvm -o - %t/u.cpp | FileCheck %s
// RUN: %clang_cc1 -std=c++2d -emit-reduced-module-interface %t/m.cppm -o %t/R.pcm
// RUN: %clang_cc1 -std=c++2d -fmodule-file=M=%t/R.pcm -fsyntax-only -verify %t/u.cpp

// CHECK: define {{.*}} @_Z3usev

//--- m.cppm
export module M;
export struct P { int a, b; };
export constexpr int classify(int x) {
  return match (x) {
    case 0 => 10;
    case 1 || 2 => 20;
    case auto y if (y > 5) => y;
    case _ => -1;
  };
}
export template <class T> constexpr int sum(T p) {
  if (case [auto a, auto b] = p)
    return a + b;
  return 0;
}
export template <class T> constexpr bool is_zero_first(T p) {
  return match(p, case [0, _]);
}
//--- u.cpp
// expected-no-diagnostics
import M;
static_assert(classify(1) == 20 && classify(9) == 9 && classify(3) == -1);
static_assert(sum(P{2, 3}) == 5);
static_assert(is_zero_first(P{0, 1}) && !is_zero_first(P{1, 1}));
int use() { return classify(2) + sum(P{1, 2}); }
