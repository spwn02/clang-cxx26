// RUN: rm -rf %t && mkdir -p %t
// RUN: split-file %s %t
// RUN: %clang_cc1 -std=c++2d -I %S/../Reflection/Inputs -I %t %t/A.cppm -emit-module-interface -o %t/A.pcm
// RUN: %clang_cc1 -std=c++2d -I %S/../Reflection/Inputs -I %t %t/B.cppm -emit-module-interface -o %t/B.pcm
// RUN: %clang_cc1 -std=c++2d -I %S/../Reflection/Inputs -I %t -fmodule-file=A=%t/A.pcm -fmodule-file=B=%t/B.pcm -fsyntax-only %t/import-AB.cpp
// RUN: %clang_cc1 -std=c++2d -I %S/../Reflection/Inputs -I %t -fmodule-file=A=%t/A.pcm -fmodule-file=B=%t/B.pcm -fsyntax-only %t/import-BA.cpp

//--- h.h
#include "token-sequence.h"
__macro init_it(int v) { return ^^{ member(\(v)) }; }
template <class T> struct DeferredCtorInit {
  int member;
  constexpr DeferredCtorInit(int v) : init_it!(v) {}
};

//--- A.cppm
module;
#include "h.h"
export module A;
export using ::DeferredCtorInit;

//--- B.cppm
module;
#include "h.h"
export module B;
export using ::DeferredCtorInit;

//--- import-AB.cpp
import A;
import B;
static_assert(DeferredCtorInit<int>(17).member == 17);

//--- import-BA.cpp
import B;
import A;
static_assert(DeferredCtorInit<int>(17).member == 17);
