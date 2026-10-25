// RUN: rm -rf %t
// RUN: mkdir %t
// RUN: split-file --leading-lines %s %t
// RUN: %clang_cc1 -std=c++26 -pedantic-errors -I%t -fsyntax-only -verify %t/main.cpp

// cwg3015: yes
// A header-name is only formed immediately after the directive name. Tokens
// following a macro that expands to nothing are ordinary tokens that are
// macro-expanded before the directive is processed.

//--- myfile.rsc.h
int from_header;

//--- main.cpp
// expected-no-diagnostics
#define EMPTY
#define X myfile
#define Y rsc.h
#include EMPTY <X.Y>
static_assert(sizeof(from_header) == sizeof(int));
