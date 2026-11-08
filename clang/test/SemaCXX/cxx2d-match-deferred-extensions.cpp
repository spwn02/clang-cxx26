// RUN: %clang_cc1 -std=c++2d -fsyntax-only -verify %s

// P2688R6 14: relational and range patterns, and-patterns, whole-value naming
// patterns and extractors are deliberately not part of R6. `or` is the only
// pattern combinator.

void deferred(int x) {
  match (x) { case int y and 3 => ; case _ => ; } // expected-error {{expected '=>' after pattern}}
  match (x) { case int z && 3 => ; case _ => ; }  // expected-error {{expected '=>' after pattern}}
  match (x) { case < 0 => ; case _ => ; }         // expected-error {{expected expression}}
  match (x) { case 1..10 => ; case _ => ; }       // expected-error {{invalid suffix '.10' on floating constant}}
}

// A pattern is only introduced by `case` inside a match selection or as a
// pattern condition (5.1). There is no block-scope pattern declaration.
struct Pair { int a, b; };
int block_scope_declaration(Pair value) {
  case [int x, int y] = value else return 0; // expected-error {{expected}}
  return 0;
}
