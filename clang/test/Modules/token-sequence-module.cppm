// RUN: rm -rf %t && mkdir -p %t
// RUN: %clang_cc1 -std=c++2d -I %S/../Reflection/Inputs -Wno-undefined-inline -emit-reduced-module-interface %s -o %t/M.pcm
// RUN: %clang_cc1 -std=c++2d -I %S/../Reflection/Inputs -Wno-undefined-inline -fmodule-file=M=%t/M.pcm -fsyntax-only -verify %S/Inputs/token-sequence-module-use.cpp

// Token contents cross a module boundary: a constexpr token sequence, and a
// consteval function returning one.
module;
#include "token-sequence.h"
export module M;

export constexpr std::meta::token_sequence exported = ^^{ x . y ( 1 , "s" ) };
export constexpr std::meta::token_sequence nested =
    ^^{ outer + ^^{ inner * \(2) } + \(42) + "text"; };
export consteval std::meta::token_sequence make() { return ^^{ return 42 ; } + ^^{ ; }; }
