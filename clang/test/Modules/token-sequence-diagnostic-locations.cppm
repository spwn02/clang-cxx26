// RUN: rm -rf %t && mkdir -p %t
// RUN: split-file %s %t
// RUN: %clang_cc1 -std=c++29 -I %S/../Reflection/Inputs -emit-module-interface %t/M.cppm -o %t/M.pcm
// RUN: %clang_cc1 -std=c++29 -I %S/../Reflection/Inputs -fmodule-file=M=%t/M.pcm -fsyntax-only -verify %t/use.cpp
// RUN: not %clang_cc1 -std=c++29 -I %S/../Reflection/Inputs -fmodule-file=M=%t/M.pcm -fsyntax-only %t/use.cpp 2>&1 | FileCheck %s --check-prefix=DIAG

//--- M.cppm
module;
#include "token-sequence.h"
export module M;
export __macro bad(int value) { return ^^{ very_long_missing_identifier + \(value) }; }
export template <class T> int invoke(T value) { return bad!(value); }

//--- use.cpp
import M;
// expected-error@M.cppm:5 {{use of undeclared identifier 'very_long_missing_identifier'}}
// expected-note@M.cppm:5 {{in expansion of expression macro 'bad'}}
int use() { return invoke(1); } // expected-note {{in instantiation of function template specialization 'invoke<int>' requested here}}

// DIAG: M.cppm:5:56: error: use of undeclared identifier 'very_long_missing_identifier'
// DIAG: M.cppm:4:44: note: expanded from macro 'bad'
// DIAG: use.cpp:4:20: note: in instantiation of function template specialization 'invoke<int>' requested here
