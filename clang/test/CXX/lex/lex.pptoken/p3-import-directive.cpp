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
// RUN: %clang_cc1 -std=c++20 -emit-module-interface %t/P.cppm -o %t/M-P.pcm
// RUN: %clang_cc1 -std=c++20 -fmodule-file=foo=%t/foo.pcm -fsyntax-only -verify %t/InExportBlock.cpp
// RUN: %clang_cc1 -std=c++20 -fmodule-file=foo=%t/foo.pcm -fsyntax-only -verify %t/InExportBlockLate.cpp
// RUN: %clang_cc1 -std=c++20 -fmodule-file=foo=%t/foo.pcm -fsyntax-only -verify %t/InExternBlockLate.cpp
// RUN: %clang_cc1 -std=c++20 -fmodule-file=M:P=%t/M-P.pcm -fsyntax-only -verify %t/PartitionInExportBlock.cpp
// RUN: %clang_cc1 -std=c++20 -fmodule-file=M:P=%t/M-P.pcm -fsyntax-only -verify %t/PartitionInExternBlock.cpp
// RUN: %clang_cc1 -std=c++20 -fmodule-file=M:P=%t/M-P.pcm -emit-module-interface %t/PartitionInExternBlock.cpp -o %t/M.pcm
// RUN: %clang_cc1 -std=c++20 -fmodule-file=M=%t/M.pcm -fmodule-file=M:P=%t/M-P.pcm -fsyntax-only -verify %t/PartitionInExternBlockImpl.cpp
// RUN: %clang_cc1 -std=c++20 -fmodule-file=foo=%t/foo.pcm -fsyntax-only -verify %t/ImportsInBlocksThenImport.cpp
// RUN: %clang_cc1 -std=c++20 -fmodule-file=foo=%t/foo.pcm -fsyntax-only -verify %t/AfterPragma.cpp
// RUN: %clang_cc1 -std=c++20 -fmodule-file=foo=%t/foo.pcm -fsyntax-only -verify %t/AfterPragmaOperator.cpp
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

//--- P.cppm
export module M:P;
export int p;

//--- InExportBlock.cpp
// expected-no-diagnostics
// [dcl.pre]: module-import-declaration is a name-declaration, so an export-declaration block may contain it
// ([module.import]: it inhabits the global namespace scope; the block does not change that).
export module M;
export {
import foo;
}

//--- ExportSameLine.cpp
struct import {};
export import w; // expected-error {{module 'w' not found}}

//--- InExportBlockLate.cpp
export module M;
int x;
export {
import foo; // expected-error {{imports must immediately follow the module declaration}}
}

//--- InExternBlockLate.cpp
export module M;
int x;
extern "C++" {
import foo; // expected-error {{imports must immediately follow the module declaration}}
}

//--- PartitionInExportBlock.cpp
// expected-no-diagnostics
export module M;
export {
import :P;
}

//--- ImportsInBlocksThenImport.cpp
// expected-no-diagnostics
// Imports in a block do not end the imports that may follow the module declaration.
export module M;
export {
import foo;
}
import foo;

//--- AfterPragma.cpp
// expected-no-diagnostics
// The line after a #pragma directive begins a line.
export module M;
#pragma pack(push, 1)
import foo;
#pragma pack(pop)
int g = f;

//--- AfterPragmaOperator.cpp
// A _Pragma operator does not end the line: 'import' is not its first token.
export module M;
_Pragma("pack(push, 1)") import foo; // expected-error {{unknown type name 'import'}}
// expected-warning@3 {{unterminated '#pragma pack (push, ...)' at end of file}}

//--- PartitionInExternBlock.cpp
// expected-no-diagnostics
// The partition belongs to the module the module-declaration names, also inside a linkage-specification, where the
// innermost scope is the implicit global module (#309).
export module M;
extern "C++" {
import :P;
}
int g = p;

//--- PartitionInExternBlockImpl.cpp
// expected-no-diagnostics
module M;
extern "C++" {
import :P;
}
int g2 = p;
