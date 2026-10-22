// RUN: %clang_cc1 -std=c++20 -fsyntax-only -verify %s
#include "Inputs/std-compare.h"

// [class.spaceship]: with a declared return type R, a usable 'a <=> b' that cannot be
// explicitly converted to R makes the synthesized three-way comparison not defined: the
// defaulted operator function is defined as deleted (CWG2703).

struct F {
  float f;
  auto operator<=>(const F&) const = default;
};
static_assert(__is_same(decltype(F{} <=> F{}), std::partial_ordering));

struct G {
  float f;
  std::strong_ordering operator<=>(const G&) const = default; // expected-warning {{explicitly defaulted three-way comparison operator is implicitly deleted}}
  // expected-note@-1 {{replace 'default' with 'delete'}}
};
bool t(G a, G b) { return a < b; } // expected-error {{object of type 'G' cannot be compared because its 'operator<=>' is implicitly deleted}}
// expected-note@16 {{explicitly defaulted function was implicitly deleted here}}

struct H {
  int i;
  std::strong_ordering operator<=>(const H&) const = default; // ok: int gives strong_ordering
};
