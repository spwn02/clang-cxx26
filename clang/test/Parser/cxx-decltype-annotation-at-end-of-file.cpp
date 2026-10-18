// A decltype-specifier that is cut off by the end of the file is annotated up
// to the last token that was consumed (it asserted in
// Preprocessor::AnnotatePreviousCachedTokens).

// RUN: rm -rf %t && split-file %s %t
// RUN: %clang_cc1 -std=c++11 -fsyntax-only -verify %t/open.cpp
// RUN: %clang_cc1 -std=c++11 -fsyntax-only -verify %t/auto.cpp
// RUN: %clang_cc1 -std=c++11 -fsyntax-only -verify %t/in-class.cpp

//--- open.cpp
int decltype(
// expected-error@* {{expected expression}}
// expected-error@* {{expected unqualified-id}}
//--- auto.cpp
int decltype(auto
// expected-error@* {{expected '(' for function-style cast or type construction}}
// expected-error@* {{expected unqualified-id}}
//--- in-class.cpp
struct S { decltype(
// expected-error@* {{expected expression}}
// expected-error@* {{expected member name or ';' after declaration specifiers}}
// expected-error@* {{expected '}'}}
// expected-note@* {{to match this '{'}}
// expected-error@* {{expected ';' after struct}}
