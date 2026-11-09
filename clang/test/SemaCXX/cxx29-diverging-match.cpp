// RUN: %clang_cc1 -std=c++2d -fsyntax-only -fcxx-exceptions -verify=cxx29 %s

// P3549 with P2688: a match arm that diverges (a call of a [[noreturn]] function, a diverging do expression, a throw)
// takes no part in the deduction of the type of the match expression.

[[noreturn]] void die();
void use(int);
namespace std { [[noreturn]] void terminate() noexcept; }

int f(int i) { int j = match (i) { case 0 => 0; case _ => die(); }; return j; }
int g(int i) { int j = match (i) { case 0 => 0; case _ => do { die(); }; }; return j; }
int h(int i) { int j = match (i) { case 0 => 0; case _ => throw 1; }; return j; }
int k(int i) { int j = match (i) { case 0 => 0; case _ => do { use(1); die(); }; }; return j; }
int m(int i) { int j = match (i) { case 0 => 0; case _ => do -> int { die(); }; }; return j; }
int n(int i) { auto j = match (i) { case 0 => 0; case _ => std::terminate(); }; return j; }
// The diverging arm may come first.
int p(int i) { auto j = match (i) { case 1 => die(); case _ => 7; }; return j; }
static_assert(__is_same(decltype(match (0) { case 0 => 1; case _ => die(); }), int));

// An arm that does not diverge still has to agree.
int q(int i) {
  return match (i) { case 0 => 0; case _ => 1.5; };
  // cxx29-error@-1 {{match handler yields type 'double', conflicting with previously deduced type 'int'}}
}
