// RUN: rm -rf %t && mkdir -p %t
// RUN: split-file %s %t
// RUN: %clang_cc1 -std=c++2d -I %S/../Reflection/Inputs -I %S/Inputs -Wno-undefined-inline -emit-module-interface %t/m1.cppm -o %t/m1.pcm
// RUN: %clang_cc1 -std=c++2d -I %S/../Reflection/Inputs -I %S/Inputs -Wno-undefined-inline -emit-module-interface %t/m2.cppm -o %t/m2.pcm
// RUN: %clang_cc1 -std=c++2d -I %S/../Reflection/Inputs -I %S/Inputs -Wno-undefined-inline -emit-module-interface -DV2 %t/m2.cppm -o %t/m2v2.pcm
// RUN: %clang_cc1 -std=c++2d -I %S/../Reflection/Inputs -I %S/Inputs -Wno-undefined-inline -fmodule-file=m1=%t/m1.pcm -fmodule-file=m2=%t/m2.pcm -fsyntax-only -verify=same %t/use.cpp
// RUN: %clang_cc1 -std=c++2d -I %S/../Reflection/Inputs -I %S/Inputs -Wno-undefined-inline -fmodule-file=m1=%t/m1.pcm -fmodule-file=m2=%t/m2v2.pcm -fsyntax-only -verify=differ %t/use.cpp

// Two modules that include the same header, with an inline function holding a
// token sequence literal and an interpolation, are not an ODR violation when
// both are imported (same-*: no diagnostic). The check does run: when one
// module's literal differs in a token it is one (differ-*).

//--- m1.cppm
module;
#include "token-sequence-odr.h"
export module m1;
export constexpr bool r1 = odr(1) == odr(1);

//--- m2.cppm
module;
#include "token-sequence-odr.h"
export module m2;
export constexpr bool r2 = odr(1) == odr(1);

//--- use.cpp
// same-no-diagnostics
#include "token-sequence-odr.h"
import m1;
import m2;
static_assert(r1 && r2);
// differ-error@*:* {{'odr' has different definitions in different modules}}
// differ-note@*:* {{but in '' found a different body}}
