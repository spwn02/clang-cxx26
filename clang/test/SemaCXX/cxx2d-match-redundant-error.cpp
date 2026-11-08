// RUN: %clang_cc1 -std=c++2d -fsyntax-only -verify %s

// P2688R6 11: non-exhaustiveness and redundancy are hard errors, not optional
// warnings, and there is no flag that turns them into anything else.

int redundant_case(bool value) {
  return match (value) {
    case _ => 0;
    case true => 1; // expected-error {{match case is redundant}}
  };
}

int redundant_or_alternative(bool value) {
  return match (value) {
    case false or false => 0; // expected-error {{or-pattern alternative is redundant}}
    case true => 1;
  };
}

struct Pair {
  int first;
  int second;
};

int redundant_pattern_declaration_else(Pair value) {
  case [int first, int second] = value else return 0; // expected-error {{match case is redundant}}
  return first + second;
}
