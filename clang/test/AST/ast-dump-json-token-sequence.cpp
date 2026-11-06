// RUN: %clang_cc1 -std=c++2d -I %S/../Reflection/Inputs -ast-dump=json -ast-dump-filter=tokens %s | FileCheck %s

#include "token-sequence.h"

constexpr int value = 42;

consteval std::meta::token_sequence tokens() {
  return ^^{ int x = \(value); };
}

// CHECK: "kind": "CXXTokenSequenceExpr"
// CHECK: "kind": "DeclRefExpr"
// CHECK: "name": "value"
