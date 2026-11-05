// RUN: c-index-test -test-load-source all -std=c++2d -I %S/../Reflection/Inputs -Wno-undefined-inline %s | FileCheck %s

#include "token-sequence.h"

constexpr std::meta::token_sequence g = ^^{ a + b };
// CHECK: token-sequence-cursors.cpp:[[@LINE-1]]:41: UnexposedExpr= Extent=[[[@LINE-1]]:41 - [[@LINE-1]]:52]
// CHECK: token-sequence-cursors.cpp:[[@LINE-2]]:41: TokenSequenceExpr= Extent=[[[@LINE-2]]:41 - [[@LINE-2]]:52]

consteval std::meta::token_sequence f(int n) { return ^^{ x = \(n) } + ^^{ ; }; }
// CHECK: token-sequence-cursors.cpp:[[@LINE-1]]:55: TokenSequenceExpr= Extent=[[[@LINE-1]]:55 - [[@LINE-1]]:69]
