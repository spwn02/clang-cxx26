// RUN: rm -rf %t
// RUN: split-file %s %t
// RUN: %clang_cc1 -std=c++20 -fsyntax-only -verify %t/NotDirective.cpp
// RUN: %clang_cc1 -std=c++20 -fsyntax-only -verify %t/Directive.cpp
// RUN: %clang_cc1 -std=c++20 -fsyntax-only -verify %t/ExportSplit.cpp
// RUN: %clang_cc1 -std=c++20 -fsyntax-only -verify %t/ExportSameLine.cpp

// [cpp.pre]: an import is a preprocessing directive only if it, or the export before it, starts a logical source line
// and it is followed on that line by a header-name, '<', an identifier or ':'.

//--- NotDirective.cpp
// expected-no-diagnostics
struct import { static int v; };
int q = import::v;   // '::'
import *p = nullptr; // '*'
import
r;                   // nothing follows on the line
int a; import s;     // not the first token of the line
#define EMPTY
EMPTY import t;      // preceded by an empty macro expansion

//--- Directive.cpp
struct import {};
import u; // expected-error {{module 'u' not found}}

//--- ExportSplit.cpp
struct import {};
export
import v; // expected-error {{module 'v' not found}}

//--- ExportSameLine.cpp
struct import {};
export import w; // expected-error {{module 'w' not found}}
