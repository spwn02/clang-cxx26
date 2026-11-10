// RUN: %clang_cc1 -std=c++2d -ast-print %s | FileCheck %s
// RUN: %clang_cc1 -std=c++2d -ast-print %s > %t.cpp
// RUN: %clang_cc1 -std=c++2d -fsyntax-only %t.cpp

// function-contract-specifiers print as written (after the virt-specifiers),
// not as an attribute list, so the output parses again.
struct B { virtual int f(int x) pre(x > 0) post(r: r > 0) { return x; } };
struct D : B { int f(int x) override pre(x > 1) { return x + 1; } };

// CHECK: virtual int f(int x) pre(x > 0) post(r: r > 0) {
// CHECK: int f(int x) override pre(x > 1) {
