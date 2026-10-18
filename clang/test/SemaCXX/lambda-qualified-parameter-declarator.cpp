// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

// A parameter declarator with a nested-name-specifier (here the global scope) in
// a lambda at namespace scope asserted in Sema::EnterDeclaratorContext: no
// scope of a class or a namespace is entered for a parameter.
auto m = [](int ::x) {}; // expected-error {{parameter declarator cannot be qualified}}
auto n = [](auto ::y) {}; // expected-error {{parameter declarator cannot be qualified}}
