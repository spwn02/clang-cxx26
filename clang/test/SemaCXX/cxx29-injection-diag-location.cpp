// RUN: %clang_cc1 -std=c++2d -fsyntax-only -verify %s

// Parsing the tokens an on_complete callback injects after a class must not
// move the location diagnostics for that class are anchored at: a missing ';'
// is reported at the closing brace of the class, not inside the injected text.

#include "Inputs/../../Reflection/Inputs/token-sequence.h"

namespace std::meta {
using info = decltype(^^::);
consteval auto queue_injection(token_sequence) -> void;
} // namespace std::meta

struct injects {
  consteval auto on_complete(std::meta::info) const -> void {
    std::meta::queue_injection(^^{
      struct injected_marker {};
      constexpr int injected_value = 1;
    });
  }
};

struct [[= injects{}]] well_formed {
  int x;
};
static_assert(injected_value == 1);

struct [[= injects{}]] missing_semicolon {
  int x;
} // expected-error {{expected ';' after struct}}

int after_the_struct;

// The same mistake without an injecting annotation is reported at the same place.
struct plain {
  int x;
} // expected-error {{expected ';' after struct}}

int after_plain;
