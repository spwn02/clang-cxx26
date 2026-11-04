// RUN: %clang_cc1 -std=c++2d -I %S/../../Reflection/Inputs -Wno-undefined-inline -emit-pch -o %t.1.ast %S/Inputs/ts1.cpp
// RUN: %clang_cc1 -std=c++2d -Wno-undefined-inline -ast-merge %t.1.ast -ast-print %s | FileCheck --check-prefix=PRINT %s
// RUN: %clang_cc1 -std=c++2d -Wno-undefined-inline -ast-merge %t.1.ast -ast-dump %s | FileCheck --check-prefix=DUMP %s

// Token sequences (literals and the values they hold) survive being imported
// into another AST: the tokens, and the interpolated expression.

// PRINT: constexpr std::meta::token_sequence global_ts = ^^{ a + b * 3 "str" 'c' 2.5 };
// PRINT: constexpr std::meta::token_sequence with_value = ^^{ x = \(42) };
// PRINT: return ^^{ int x = 1 ; return x ; } + ^^{ ; };

// DUMP: CXXTokenSequenceExpr {{.*}} tokens:8 'a + b * 3 "str" 'c' 2.5'
// DUMP: field: TokenSequence 3 'x = \(...)'
