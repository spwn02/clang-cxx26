// RUN: %clang_cc1 -std=c++2d -I %S/../Reflection/Inputs -emit-pch -o %t.pch -verify %s

// A non-empty token sequence round-trips through a PCH or module as an empty
// one (#387): building one with such a sequence in it says so.
#include "token-sequence.h"
using std::meta::token_sequence;

consteval token_sequence make() {
  return ^^{ int x; }; // expected-warning {{a non-empty token sequence is not preserved in a precompiled header or module}}
}

// An empty one is fine.
consteval token_sequence nothing() { return ^^{ }; }
