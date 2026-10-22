// RUN: %clang_cc1 -std=c++20 -emit-module-interface %s -o %t.pcm -verify

// P3034R1: an identifier of a module-name is never macro-expanded, and it
// shall not be defined as an object-like macro.
module;
#define MODULE_NAME not_the_module_name

export module MODULE_NAME; // expected-error {{the identifier 'MODULE_NAME' in a module name cannot be defined as a macro name or be followed by '('}}

export int value;
