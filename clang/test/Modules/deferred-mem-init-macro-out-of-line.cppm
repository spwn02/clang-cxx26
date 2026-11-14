// RUN: rm -rf %t && mkdir -p %t
// RUN: split-file %s %t
// RUN: %clang_cc1 -std=c++2d -I %S/../Reflection/Inputs -I %t -emit-module-interface %t/A-decl.cppm -o %t/A-decl.pcm
// RUN: %clang_cc1 -std=c++2d -I %S/../Reflection/Inputs -I %t -fmodule-file=A:decl=%t/A-decl.pcm -emit-module-interface %t/A-impl.cppm -o %t/A-impl.pcm
// RUN: %clang_cc1 -std=c++2d -I %S/../Reflection/Inputs -I %t -fmodule-file=A:decl=%t/A-decl.pcm -fmodule-file=A:impl=%t/A-impl.pcm -emit-module-interface %t/A.cppm -o %t/A.pcm
// RUN: %clang_cc1 -std=c++2d -fmodule-file=A=%t/A.pcm -fmodule-file=A:decl=%t/A-decl.pcm -fmodule-file=A:impl=%t/A-impl.pcm -fsyntax-only %t/import.cpp

//--- A-decl.cppm
export module A:decl;
export template <class T> struct D {
  int member;
  constexpr D(int v);
};

//--- hmacro.h
#include "token-sequence.h"
__macro init_it(int v) { return ^^{ member(\(v)) }; }

//--- A-impl.cppm
module;
#include "hmacro.h"
export module A:impl;
import :decl;
template <class T> constexpr D<T>::D(int v) : init_it!(v) {}

//--- A.cppm
export module A;
export import :decl;
export import :impl;

//--- import.cpp
import A;
static_assert(D<int>(17).member == 17);
