// RUN: %clang_cc1 -std=c++2d -I %S/../Reflection/Inputs -Wno-undefined-inline -fsyntax-only -verify -verify-ignore-unexpected=note %s

#include "token-sequence.h"

// Mistakes with token sequence injection are diagnosed, not crashed on.

namespace std::meta {
using info = decltype(^^::);
consteval auto queue_injection(token_sequence) -> void;
consteval auto queue_injection(info target_ns, token_sequence) -> void;
consteval auto macro_expansion_context() -> info;
} // namespace std::meta

// The operand of queue_injection must be a token sequence.
consteval void bad_operand() {
  std::meta::queue_injection(42); // expected-error {{no matching function for call to 'queue_injection'}}
}

// The target must be a namespace.
struct C {};
consteval void bad_target() {
  std::meta::queue_injection(^^C, ^^{ int x; });
}
consteval { bad_target(); } // expected-error {{evaluating expression of a consteval block must be a constant expression}}
// expected-note@-3 {{first argument of two-argument queue_injection must be a namespace reflection}}

// An interpolation must be a constant expression where it is evaluated.
int g;
constexpr auto not_constant = ^^{ \(g) }; // expected-error {{constexpr variable 'not_constant' must be initialized by a constant expression}}
// expected-note@-1 {{read of non-const variable 'g' is not allowed in a constant expression}}

// A macro is found where it is defined, and cannot be virtual.
int use = early!(1); // expected-error {{use of undeclared expression macro 'early'}}
__macro early(int x) { return ^^{ 1 }; }
struct V {
  virtual __macro vm(int x) { return ^^{ 1 }; } // expected-error {{a non-static member expression macro must have an explicit object parameter}} \
                                                // expected-error {{an expression macro cannot be virtual}}
};

// A macro returns a token sequence.
__macro not_a_sequence(int x) { return 5; } // expected-error {{no viable conversion from returned value of type 'int' to function return type 'std::meta::token_sequence'}}

// Recursion stops at the maximum depth.
__macro rec(int x) { return ^^{ rec!(\(x)) }; }
int r = rec!(1); // expected-error {{recursive expansion of expression macro 'rec' exceeded maximum depth of 256}}
