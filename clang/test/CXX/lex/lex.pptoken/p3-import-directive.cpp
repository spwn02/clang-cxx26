// RUN: rm -rf %t
// RUN: split-file %s %t
// RUN: %clang_cc1 -std=c++20 -fsyntax-only -verify %t/NotDirective.cpp
// RUN: %clang_cc1 -std=c++20 -fsyntax-only -verify %t/Directive.cpp
// RUN: %clang_cc1 -std=c++20 -emit-module-interface %t/foo.cppm -o %t/foo.pcm
// RUN: %clang_cc1 -std=c++20 -fmodule-file=foo=%t/foo.pcm -fsyntax-only -verify %t/ExportSplit.cpp
// RUN: %clang_cc1 -std=c++20 -fmodule-file=foo=%t/foo.pcm -fsyntax-only -verify %t/ExportSameLine2.cpp
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
int b; \
import s2;           // one logical line

//--- Directive.cpp
struct import {};
import u; // expected-error {{module 'u' not found}}

//--- foo.cppm
export module foo;
export int f;

//--- ExportSplit.cpp
// The import is a directive; the export on the line before it is an ordinary token, which cannot export it.
export module M;
export // expected-error {{'export' is not the export-keyword of this import directive; it must be on the same logical source line}}
import foo;
int g = f;

//--- ExportSameLine2.cpp
// expected-no-diagnostics
export module M;
export import foo;
int g = f;

//--- ExportSameLine.cpp
struct import {};
export import w; // expected-error {{module 'w' not found}}
