// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

// The parameter of a lambda in a generic lambda has the type of a class that is
// declared in the call operator itself ('enum y'): numbering the lambda
// mangled the call operator for ever (stack overflow in the Itanium mangler).
auto lam=[](auto){[](num y)mutable{}}r<lam(2)
// expected-error@* {{unknown type name 'num'}}
// expected-error@* {{ISO C++ forbids forward references to 'enum' types}}
// expected-error@* 2 {{variable has incomplete type 'enum y'}}
// expected-note@* 2 {{forward declaration of 'y'}}
// expected-error@* 2 {{expected ';'}}
// expected-note@* {{in instantiation of function template specialization}}
// expected-warning@* {{expression result unused}}
// expected-error@* {{expected '>'}}
// expected-note@* {{to match this '<'}}
