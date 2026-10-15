// RUN: rm -rf %t
// RUN: split-file %s %t
// RUN: %clang_cc1 -std=c++20 -fsyntax-only -verify %t/NotDirective.cpp
// RUN: %clang_cc1 -std=c++20 -fsyntax-only -verify %t/Directive.cpp
// RUN: %clang_cc1 -std=c++20 -emit-module-interface %t/foo.cppm -o %t/foo.pcm
// RUN: %clang_cc1 -std=c++20 -fmodule-file=foo=%t/foo.pcm -fsyntax-only -verify %t/ExportSplit.cpp
// RUN: %clang_cc1 -std=c++20 -fmodule-file=foo=%t/foo.pcm -fsyntax-only -verify %t/ExportSameLine2.cpp
// RUN: %clang_cc1 -std=c++20 -fmodule-file=foo=%t/foo.pcm -fsyntax-only -verify %t/ExportSplice.cpp
// RUN: %clang_cc1 -std=c++20 -fmodule-file=foo=%t/foo.pcm -fsyntax-only -verify %t/ExportLine.cpp
// RUN: %clang_cc1 -std=c++20 -fmodule-file=foo=%t/foo.pcm -fsyntax-only -verify %t/InNamespace.cpp
// RUN: %clang_cc1 -std=c++20 -fmodule-file=foo=%t/foo.pcm -fsyntax-only -verify %t/InClass.cpp
// RUN: %clang_cc1 -std=c++20 -fmodule-file=foo=%t/foo.pcm -fsyntax-only -verify %t/InFunction.cpp
// RUN: %clang_cc1 -std=c++20 -fmodule-file=foo=%t/foo.pcm -fsyntax-only -verify %t/InExportBlock.cpp
// RUN: %clang_cc1 -std=c++20 -fmodules -fmodules-cache-path=%t/cache -fmodule-map-file=%t/module.modulemap \
// RUN:   -fmodule-file=foo=%t/foo.pcm -fsyntax-only -verify %t/AfterInclude.cpp
// RUN: %clang_cc1 -std=c++20 -fmodules -fmodules-cache-path=%t/cache -fmodule-map-file=%t/module.modulemap \
// RUN:   -fmodule-file=foo=%t/foo.pcm -fsyntax-only -verify %t/ExportAfterInclude.cpp
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

//--- hdr.h
#ifndef HDR_H
#define HDR_H
int hdr;
#endif

//--- module.modulemap
module "hdr.h" { header "hdr.h" export * }

//--- AfterInclude.cpp
// expected-no-diagnostics
// The lexer does not flag the first token after a #include that became an annotation token as the start of a line.
#include "hdr.h"
import foo;
int g = f + hdr;

//--- ExportAfterInclude.cpp
// The export-import directive is recognized (it is diagnosed as an export outside a module interface, not as an identifier).
#include "hdr.h"
export import foo; // expected-error {{export declaration can only be used within a module interface}}

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

//--- ExportSplice.cpp
// expected-no-diagnostics
// A line splice keeps 'export' and 'import' on one logical line.
export module M;
export \
import foo;
int g = f;

//--- ExportLine.cpp
export module M;
export // expected-error {{'export' is not the export-keyword of this import directive; it must be on the same logical source line}}
#line 100
import foo;

//--- InNamespace.cpp
export module M;
namespace N { // expected-note {{begins here}}
import foo; // expected-error {{import of module 'foo' appears within namespace 'N'}}
}

//--- InClass.cpp
export module M;
struct S {
import foo; // expected-error {{expected member name or ';' after declaration specifiers}}
};

//--- InFunction.cpp
export module M;
void g() {
import foo; // expected-error {{expected expression}}
}

//--- InExportBlock.cpp
// expected-no-diagnostics
// [module.import]: a module-import-declaration inhabits the global namespace scope; an export block does not change that.
export module M;
export {
import foo;
}

//--- ExportSameLine.cpp
struct import {};
export import w; // expected-error {{module 'w' not found}}
