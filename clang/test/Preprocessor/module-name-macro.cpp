// RUN: split-file %s %t
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify=ok %t/attr.cppm
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %t/object.cppm
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %t/partition.cppm
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %t/qualifier.cppm
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %t/function.cppm
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %t/dot.cppm
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify=ok %t/not-called.cppm

// [cpp.module], P3034R1: no identifier of a module-name or partition is a macro name, and
// the pp-tokens after the name are macro-expanded and must begin with ';' or '['.

//--- attr.cppm
// ok-warning@+3 {{unknown attribute 'vendor::shiny_module' ignored}}
module;
#define MOD_ATTR [[vendor::shiny_module]]
export module M MOD_ATTR ;
int x;

//--- object.cppm
module;
#define MODNAME foo
export module MODNAME; // expected-error {{the identifier 'MODNAME' in a module name cannot be defined as a macro name}}
int x;

//--- partition.cppm
module;
#define PART part
export module M:PART; // expected-error {{the identifier 'PART' in a module name cannot be defined as a macro name}}
int x;

//--- qualifier.cppm
module;
#define Q q
export module Q.m; // expected-error {{the identifier 'Q' in a module name cannot be defined as a macro name}}
int x;

//--- function.cppm
module;
#define F(x) x
export module F(a); // expected-error {{the identifier 'F' in a module name cannot be defined as a macro name}} expected-error {{expected ';' after module name}} expected-error {{a type specifier is required}}
int x;

//--- dot.cppm
module;
#define DOT_BAR .bar
export module foo DOT_BAR; // expected-error {{the preprocessing token after a module name must be ';' or '['}}
int x;

//--- not-called.cppm
// ok-no-diagnostics
module;
#define F(x) x
export module F; // function-like macro name not followed by '('
int x;
