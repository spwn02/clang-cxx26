// RUN: %clang_cc1 -std=c++29 -pedantic -Werror -I %S/../Reflection/Inputs -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++29 -I %S/../Reflection/Inputs -ast-print %s | FileCheck %s

// `macro` is a provisional spelling of `__macro` (P4380): a keyword where an
// expression-macro declaration starts, an ordinary identifier elsewhere, and
// the same declaration as `__macro`.

#include "token-sequence.h"

macro one(int x) { return ^^{ (\(x) + 1) }; }
__macro two(int x) { return ^^{ (\(x) + 2) }; }
template <class T> macro fwd(T &&t) { return ^^{ \(t) }; }
struct S {
  macro bump(this S &self, int n) { return ^^{ (\(self) + \(n)) }; }
  int macro;
};

// `macro` is still an identifier where no declaration starts.
int identifiers(S s) {
  int macro = s.macro;
  return macro + one!(1) + two!(2);
}
// expected-no-diagnostics

// Both spellings are the same declaration.
// CHECK: token_sequence one(int x)
// CHECK: token_sequence two(int x)
