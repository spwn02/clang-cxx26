// RUN: %clang_cc1 -std=c++2d -ast-print %s | FileCheck %s

template <class T> concept C = requires(T t) {
  { t.f() } noexcept(noexcept(t.g()));
  { t.f() } noexcept;
};
// CHECK: { t.f() } noexcept(noexcept(t.g()));
// CHECK: { t.f() } noexcept;
