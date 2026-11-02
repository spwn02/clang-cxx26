// Do expressions (P2806) survive a precompiled header round trip, in particular
// the init-capture variables, whose flag bit does not fit the abbreviated DECL_VAR
// record (the writer used to assert "High bits set!").

// RUN: rm -rf %t && mkdir %t && split-file %s %t
// RUN: %clang_cc1 -std=c++29 -fdo-expressions -x c++-header -emit-pch -o %t/h.pch %t/h.h
// RUN: %clang_cc1 -std=c++29 -fdo-expressions -include-pch %t/h.pch -fsyntax-only -verify %t/u.cpp
// RUN: %clang_cc1 -std=c++29 -fdo-expressions -include-pch %t/h.pch -ast-dump-all -ast-dump-filter=pick %t/u.cpp | FileCheck %s
// Also with templates instantiated while building the PCH.
// RUN: %clang_cc1 -std=c++29 -fdo-expressions -fpch-instantiate-templates -x c++-header -emit-pch -o %t/h2.pch %t/h.h
// RUN: %clang_cc1 -std=c++29 -fdo-expressions -include-pch %t/h2.pch -fsyntax-only -verify %t/u.cpp

// CHECK: DoExpr
// CHECK: VarDecl {{.*}} q
// CHECK: DoReturnStmt

//--- h.h
inline int pick(bool b) { return do [q = 1] { if (b) do_return q; do_return 2; }; }
template <class T> constexpr T twice(T v) { return do [k = v] -> T { do_return k + k; }; }
constexpr int sum4 = do { int s = 0; for (int i = 0; i < 4; ++i) s += i; do_return s; };
struct S { int v; constexpr S(int x) : v(do [m = x] { do_return m * 3; }) {} };
//--- u.cpp
// expected-no-diagnostics
static_assert(sum4 == 6);
static_assert(twice(21) == 42);
static_assert(S(5).v == 15);
int use() { return pick(true) + twice(1); }
