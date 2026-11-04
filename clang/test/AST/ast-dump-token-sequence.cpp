// RUN: %clang_cc1 -std=c++2d -I %S/../Reflection/Inputs -ast-dump -ast-dump-filter=tokens %s | FileCheck %s

#include "token-sequence.h"

constexpr int value = 42;

consteval std::meta::token_sequence tokens() {
  return ^^{ int x = \(value); };
}

// CHECK: FunctionDecl {{.*}} tokens
// CHECK: CXXTokenSequenceExpr
// CHECK-NEXT: `-DeclRefExpr {{.*}} 'const int' lvalue Var {{.*}} 'value' 'const int'
