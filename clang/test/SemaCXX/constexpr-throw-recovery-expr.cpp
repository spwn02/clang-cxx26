// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

// A throw whose operand failed to build is a RecoveryExpr, which is
// value-dependent. Evaluating it used to assert in Evaluate().

constexpr int f(int c) {
  if (c == 0)
    throw undeclared_fn(1); // expected-error {{use of undeclared identifier 'undeclared_fn'}} \
                            // expected-note {{subexpression not valid in a constant expression}}
  return c;
}
constexpr int v = f(0); // expected-error {{constexpr variable 'v' must be initialized by a constant expression}} \
                        // expected-note {{in call to 'f(0)'}}

struct F { explicit F(int); }; // expected-note 3 {{candidate constructor}}
constexpr int g(int c) {
  if (c == 0)
    throw F(); // expected-error {{no matching constructor for initialization of 'F'}} \
               // expected-note {{subexpression not valid in a constant expression}}
  return c;
}
constexpr int w = g(0); // expected-error {{constexpr variable 'w' must be initialized by a constant expression}} \
                        // expected-note {{in call to 'g(0)'}}
