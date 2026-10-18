// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

// A variable of a class that has no definition, in the body of a generic lambda
// (the type of a malformed declaration): QualType::isNonConstantStorage asked
// the class for its properties and asserted.
m=[](auto){template<>truct a static constexpr v=1
// expected-error@-1 {{a type specifier is required for all declarations}}
// expected-error@-2 {{unknown type name 'truct'}}
// expected-error@-3 {{extraneous 'template<>' in declaration of variable 'v'}}
// expected-error@* {{expected ';' at end of declaration}}
// expected-error@* {{expected '}'}}
// expected-note@* {{to match this '{'}}
// expected-error@* {{expected ';' after top level declarator}}
