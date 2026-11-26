// RUN: %clang_cc1 -std=c++23 -fsyntax-only -verify %s

// C++23 does not have contracts or the P4451 assertion-control specifier.
void p4451_requires_cxx26() pre<std::contracts::controls::enforced>(true); // expected-error {{use of undeclared identifier 'std'}} expected-error {{expected function body after function declarator}}
