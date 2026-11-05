// RUN: %clang_cc1 -std=c++2d -I %S/../Reflection/Inputs -Wno-undefined-inline -ast-print %s | FileCheck %s

#include "token-sequence.h"

// The printer shows the tokens of a literal (not "^^{...}"), an interpolation
// as \(e), and a compiler-made bool constant as true/false.
consteval std::meta::token_sequence f(int n) {
  return ^^{ int x = \(n) + 1; return x; };
}
// CHECK: return ^^{ int x = \(n) + 1 ; return x ; };
constexpr std::meta::token_sequence g = ^^{ a . b ( 1 , "s" ) };
// CHECK: ^^{ a . b ( 1 , "s" ) }
