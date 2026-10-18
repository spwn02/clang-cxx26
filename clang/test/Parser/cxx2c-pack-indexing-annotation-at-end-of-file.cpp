// A pack-indexing type cut off by the end of the file is annotated up to the
// last token that was consumed (it asserted in
// Preprocessor::AnnotatePreviousCachedTokens during the tentative parse of the
// parameters).

// RUN: rm -rf %t && split-file %s %t
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %t/index.cpp
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %t/empty.cpp

//--- index.cpp
// expected-error@+5 {{expected ']'}}
// expected-note@+4 {{to match this '['}}
// expected-error@+3 {{expected ')'}}
// expected-note@+2 {{to match this '('}}
// expected-error@+1 {{expected function body after function declarator}}
template<class...Ts> void f(Ts...[0
//--- empty.cpp
// expected-error@+6 {{expected expression}}
// expected-error@+5 {{expected ']'}}
// expected-note@+4 {{to match this '['}}
// expected-error@+3 {{expected ')'}}
// expected-note@+2 {{to match this '('}}
// expected-error@+1 {{expected function body after function declarator}}
template<class...Ts> void f(Ts...[
