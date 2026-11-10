// RUN: %clang_cc1 -std=c++2d -triple x86_64-linux-gnu -emit-llvm -o - %s | FileCheck %s
// Provisional Itanium encoding (no ABI exists yet): a plain noexcept is 'N',
// a conditional one is 'Nc <expression>'.

template <bool B> struct S { };
template <typename T> void plain(S<requires(T t) { { t } noexcept; }>) { }
template <typename T, bool B> void cond(S<requires(T t) { { t } noexcept(B); }>) { }

void use() {
  plain<int>(S<true>());
  cond<int, true>(S<true>());
}

// CHECK: define {{.*}} @_Z5plainIiEv1SIXrQT__Xfp_NEEE
// CHECK: define {{.*}} @_Z4condIiLb1EEv1SIXrQT__Xfp_NcT0_EEE
