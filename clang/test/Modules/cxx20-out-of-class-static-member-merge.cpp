// The definition of a static data member outside its class, in a header that a
// module unit includes in its purview after importing a module that includes
// it in its global module fragment: the redeclarations are merged, and the one
// of the purview has no access specifier of its own (Decl::getAccess asserted
// "Access specifier is AS_none inside a record decl").

// RUN: rm -rf %t && split-file %s %t
// RUN: %clang_cc1 -std=c++20 -I%t -emit-reduced-module-interface %t/m.cppm -o %t/m.pcm
// RUN: %clang_cc1 -std=c++20 -I%t -fmodule-file=M=%t/m.pcm -fsyntax-only -verify %t/u.cpp

//--- po.h
#pragma once
struct PO { constexpr PO() {} static const PO eq; };
inline constexpr PO PO::eq{};
//--- m.cppm
module;
#include "po.h"
export module M;
export using ::PO;
//--- u.cpp
export module U;
import M;
#include "po.h"
// expected-error@po.h:2 {{declaration of 'PO' in module U follows declaration in the global module}}
// expected-note@po.h:2 {{previous declaration is here}}
PO x = PO::eq; // expected-error {{reference to 'eq' is ambiguous}}
// expected-note@po.h:3 2 {{candidate found by name lookup is 'PO::eq'}}
