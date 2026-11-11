// RUN: %clang_cc1 %s -std=c++2d -fsyntax-only -verify

// A member splice (`base.\(refl)`) in an injected member function whose base is
// wrong is diagnosed, not crashed on: the base was an error (an undeclared
// identifier), a type that is not a class, or valid.

#include "Inputs/token-sequence.h"

namespace std::meta {
using info = decltype(^^::);
} // namespace std::meta
using std::meta::info;
using std::meta::token_sequence;

struct Other {
  int x;
};

// The base names nothing: only the undeclared identifier is reported.
struct undeclared_base {
  consteval auto inject_members(info) const -> token_sequence {
    constexpr info m = ^^Other::x;
    return ^^{ int get() const { return object.\(m); } }; // expected-error {{use of undeclared identifier 'object'}}
  }
};
struct [[=undeclared_base{}]] A1 {};

// The same through '->'.
struct undeclared_arrow_base {
  consteval auto inject_members(info) const -> token_sequence {
    constexpr info m = ^^Other::x;
    return ^^{ int get() const { return object->\(m); } }; // expected-error {{use of undeclared identifier 'object'}}
  }
};
struct [[=undeclared_arrow_base{}]] A2 {};

// The base has a type that is not a class.
struct int_base {
  consteval auto inject_members(info) const -> token_sequence {
    constexpr info m = ^^Other::x;
    return ^^{ int get(int i) const { return i.\(m); } }; // expected-error {{member reference base type 'int' is not a structure or union}}
  }
};
struct [[=int_base{}]] A3 {};

// A valid base still works.
struct good_base {
  consteval auto inject_members(info) const -> token_sequence {
    constexpr info m = ^^Other::x;
    return ^^{ static constexpr int get(Other o) { return o.\(m); } };
  }
};
struct [[=good_base{}]] A4 {};
static_assert(A4::get(Other{7}) == 7);
