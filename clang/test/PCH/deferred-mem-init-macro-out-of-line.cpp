// RUN: rm -rf %t && mkdir -p %t
// RUN: split-file %s %t
// RUN: %clang_cc1 -std=c++2d -I %S/../Reflection/Inputs -I %t -emit-pch -x c++-header %t/h1.h -o %t/p1.pch
// RUN: %clang_cc1 -std=c++2d -I %S/../Reflection/Inputs -I %t -include-pch %t/p1.pch -emit-pch -x c++-header %t/h2.h -o %t/p2.pch
// RUN: %clang_cc1 -std=c++2d -I %S/../Reflection/Inputs -I %t -include-pch %t/p2.pch -fsyntax-only %t/u.cpp

//--- h1.h
#include "token-sequence.h"
__macro init_it(int v) { return ^^{ member(\(v)) }; }
template <class T> struct D {
  int member;
  constexpr D(int v);
};

//--- h2.h
template <class T> constexpr D<T>::D(int v) : init_it!(v) {}

//--- u.cpp
static_assert(D<int>(17).member == 17);
