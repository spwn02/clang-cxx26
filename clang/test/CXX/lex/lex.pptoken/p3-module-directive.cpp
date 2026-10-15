// RUN: rm -rf %t
// RUN: split-file %s %t
// RUN: %clang_cc1 -std=c++20 -fsyntax-only -verify %t/NotDirective.cpp
// RUN: %clang_cc1 -std=c++20 -fsyntax-only -verify %t/MidLine.cpp
// RUN: %clang_cc1 -std=c++20 -fsyntax-only -verify %t/Misplaced.cpp
// RUN: %clang_cc1 -std=c++20 -emit-module-interface -verify %t/Fragment.cpp -o %t/Fragment.pcm
// RUN: %clang_cc1 -std=c++20 -emit-module-interface -verify %t/Private.cpp -o %t/Private.pcm
// RUN: %clang_cc1 -std=c++20 -emit-module-interface -verify %t/EmptyMacro.cpp -DMACRO= -o %t/EmptyMacro.pcm

// [cpp.pre]: a module declaration is a preprocessing directive only if module, or the export before it, starts a
// logical source line and is followed on that line by an identifier, ':' or ';'. #308

//--- NotDirective.cpp
// expected-no-diagnostics
struct module { static int v; };
int q = module::v; // '::'
module
x;                 // nothing follows on the line

//--- MidLine.cpp
// expected-no-diagnostics
struct module {};
int a; module m;   // not the first token of the line: a declaration of m

//--- Misplaced.cpp
export module M;
struct module {};
namespace N {
module b; // expected-error {{module declaration can only appear at the top level}}
}

//--- Fragment.cpp
// expected-no-diagnostics
module;
#include "hdr.h"
export module M;
int g;

//--- hdr.h
int h;

//--- Private.cpp
// expected-no-diagnostics
export module M;
export int pub;
module :private;
int priv;

//--- EmptyMacro.cpp
// An empty macro expansion on the line before does not matter: export starts its line.
MACRO // expected-note {{add 'module;' to the start of the file to introduce a global module fragment}}
export module M; // expected-error {{module declaration must occur at the start of the translation unit}}
