// RUN: c-index-test -test-load-source all -std=c++2d %s | FileCheck %s

// A splice used as a nested-name-specifier is a child of the expression that
// contains it (libclang used to abort on this kind of specifier).
namespace ns { constexpr int q = 5; }
constexpr int v = [:^^ns:]::q;
// CHECK: token-sequence-splice-scope.cpp:[[@LINE-1]]:15: VarDecl=v:[[@LINE-1]]:15 (Definition)
