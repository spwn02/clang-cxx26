// RUN: %clang_cc1 -std=c++26 -fsyntax-only -ast-dump %s | FileCheck %s
//
// The type of a dependent splice is uniqued; each use keeps the location of
// its own tokens.

using info = decltype(^^int);
template <info R> int first() { return 1 + (typename [:R:]{}); }
template <info R> int second() { return 2 + (typename [:R:]{}); }

// (A stale location would print `line:7:...` for the second one.)
// CHECK: FunctionDecl {{.*}} first
// CHECK: CXXUnresolvedConstructExpr {{.*}} <col:45, col:60>
// CHECK: FunctionDecl {{.*}} second
// CHECK: CXXUnresolvedConstructExpr {{.*}} <col:46, col:61>
