// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

void no_contracts_header() pre<enforced>(true); // expected-error {{use of undeclared identifier 'enforced'}}
