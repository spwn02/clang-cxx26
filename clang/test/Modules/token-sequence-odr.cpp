// RUN: rm -rf %t && mkdir -p %t
// RUN: split-file %s %t
// RUN: %clang_cc1 -std=c++2d -I %S/../Reflection/Inputs -I %S/Inputs -Wno-undefined-inline -emit-reduced-module-interface %t/m1.cppm -o %t/m1.pcm
// RUN: %clang_cc1 -std=c++2d -I %S/../Reflection/Inputs -I %S/Inputs -Wno-undefined-inline -emit-reduced-module-interface %t/m2.cppm -o %t/m2.pcm
// RUN: %clang_cc1 -std=c++2d -I %S/../Reflection/Inputs -I %S/Inputs -Wno-undefined-inline -fmodule-file=m1=%t/m1.pcm -fmodule-file=m2=%t/m2.pcm -fsyntax-only -verify %t/use.cpp

// Two modules that include the same header (with an inline function that
// holds a token sequence literal with an interpolation) are not an ODR
// violation when both are imported.

//--- m1.cppm
module;
#include "token-sequence-odr.h"
export module m1;
export constexpr int one = 1;

//--- m2.cppm
module;
#include "token-sequence-odr.h"
export module m2;
export constexpr int two = 2;

//--- use.cpp
// expected-no-diagnostics
#include "token-sequence-odr.h"
import m1;
import m2;
static_assert(one + two == 3);
static_assert(odr(1) == odr(1));
static_assert(odr_use() == odr_use());
