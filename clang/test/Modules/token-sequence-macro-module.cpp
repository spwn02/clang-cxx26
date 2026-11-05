// RUN: rm -rf %t && mkdir -p %t
// RUN: split-file %s %t
// RUN: %clang_cc1 -std=c++2d -I %S/../Reflection/Inputs -Wno-undefined-inline -emit-reduced-module-interface %t/macros.cppm -o %t/macros.pcm
// RUN: %clang_cc1 -std=c++2d -I %S/../Reflection/Inputs -Wno-undefined-inline -fmodule-file=macros=%t/macros.pcm -fsyntax-only -verify %t/use.cpp

// An expression macro defined in a module expands in the file that imports
// it: with an expression parameter, with a raw token sequence parameter, and
// from inside a template instantiated by the importer.

//--- macros.cppm
module;
#include "token-sequence.h"
export module macros;

export __macro inc(int x) { return ^^{ (\(x) + 1) }; }
export __macro keep(std::meta::token_sequence t) { return ^^{ (\(t)) }; }
export template <class T> constexpr T addone(T t) { return inc!(t); }

//--- use.cpp
// expected-no-diagnostics
#include "token-sequence.h"
import macros;
static_assert(inc!(41) == 42);
static_assert(keep!(6 * 7) == 42);
static_assert(addone(41) == 42);
