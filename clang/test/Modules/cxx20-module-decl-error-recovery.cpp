// A module declaration after a malformed module declaration or private module
// fragment: the module state is not the one of a valid file, which was
// diagnosed already (it asserted in Sema::ActOnModuleDecl).

// RUN: rm -rf %t && split-file %s %t
// RUN: %clang_cc1 -std=c++20 -fsyntax-only -verify %t/gmf.cpp
// RUN: %clang_cc1 -std=c++20 -fsyntax-only -verify %t/missing-semi.cpp

//--- gmf.cpp
module;
module :private
export module S
// expected-error@* {{expected ';' after private module fragment declaration}}
// expected-error@* {{private module fragment declaration with no preceding module declaration}}
// expected-error@* {{expected ';' after module name}}
// expected-error@* {{module declaration must occur at the start of the translation unit}}
// expected-note@* {{add 'module;' to the start of the file to introduce a global module fragment}}
//--- missing-semi.cpp
module e
module :private
export module d
// expected-error@* {{expected ';' after module name}}
// expected-error@* {{module 'e' not found}}
