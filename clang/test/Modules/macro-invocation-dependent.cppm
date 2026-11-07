// A dependent expression-macro invocation in a named module keeps the
// environment its unparsed arguments are parsed in: the context, the template
// parameters in scope and the explicit template arguments of a member
// invocation (#502; the same record as the PCH one, see
// PCH/macro-invocation-dependent.cpp).

// RUN: rm -rf %t && mkdir %t && split-file %s %t
// RUN: %clang_cc1 -std=c++2d -I %S/../Reflection/Inputs -emit-module-interface %t/m.cppm -o %t/M.pcm
// RUN: %clang_cc1 -std=c++2d -I %S/../Reflection/Inputs -fmodule-file=M=%t/M.pcm -fsyntax-only -verify %t/u.cpp
// RUN: %clang_cc1 -std=c++2d -I %S/../Reflection/Inputs -emit-reduced-module-interface %t/m.cppm -o %t/R.pcm
// RUN: %clang_cc1 -std=c++2d -I %S/../Reflection/Inputs -fmodule-file=M=%t/R.pcm -fsyntax-only -verify %t/u.cpp

//--- m.cppm
module;
#include "token-sequence.h"
export module M;

export struct Holder {
  int v;
  __macro bump(this Holder &self, int n) { return ^^{ (\(self).v += \(n)) }; }
  template <class U>
  __macro tm(this Holder &self, int n) { return ^^{ (\(self).v += sizeof(U) * \(n)) }; }
};
export __macro plus_one(int x) { return ^^{ (\(x) + 1) }; }

export template <class T> constexpr int use_member(T t) { t.bump!(1); return t.v; }
export template <class T> constexpr int use_member_tmpl(T t) { t.tm!<int>(2); return t.v; }
export template <class T> constexpr int use_free(T t) { return plus_one!(t.v); }

//--- u.cpp
// expected-no-diagnostics
import M;

static_assert(use_member(Holder{10}) == 11);
static_assert(use_member_tmpl(Holder{10}) == 18);
static_assert(use_free(Holder{10}) == 11);
