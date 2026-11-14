// RUN: rm -rf %t && mkdir -p %t
// RUN: split-file %s %t
// RUN: %clang_cc1 -std=c++2d -I %S/../Reflection/Inputs -emit-module-interface %t/deferred.cppm -o %t/deferred.pcm
// RUN: %clang_cc1 -std=c++2d -I %S/../Reflection/Inputs -fmodule-file=deferred=%t/deferred.pcm -fsyntax-only %t/use.cpp

//--- deferred.cppm
module;
#include "token-sequence.h"
__macro init_it(int v) { return ^^{ member(\(v)) }; }
export module deferred;
export template <class T> struct DeferredCtorInit {
  int member;
  constexpr DeferredCtorInit(int v) : init_it!(v) {}
};

//--- use.cpp
import deferred;
static_assert(DeferredCtorInit<int>(17).member == 17);
int main() { DeferredCtorInit<int> value(17); return value.member != 17; }
