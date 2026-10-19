// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

// The typo 'v' of the condition is corrected while the full expression of the
// static_assert is finished, to a member of a class that is not in scope; the
// template argument list does not fit it, and the expression that is left is a
// recovery of the error, which is value-dependent: nothing is evaluated.
struct c { static int v; }; // expected-note {{'c::v' declared here}} expected-note {{class template declared here}}
static_assert(v<>); // expected-error {{use of undeclared identifier 'v'; did you mean 'c::v'?}} \
                    // expected-error {{'v' is expected to be a non-type template, but instantiated to a class template}}
