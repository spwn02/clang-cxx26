// RUN: %clang_cc1 -std=c++2d -fsyntax-only -verify -Wunused-value -I %S/../Reflection/Inputs %s
// expected-no-diagnostics

// A macro invocation is an expression like its expansion: noexcept sees what
// the expansion does, also when the invocation is dependent.
#include "token-sequence.h"

int may_throw(int);
int no_throw(int) noexcept;

__macro safe(int x) { return ^^{ (\(x) + 1) }; }
__macro risky(int x) { return ^^{ may_throw(\(x)) }; }
__macro quiet(int x) { return ^^{ no_throw(\(x)) }; }

static_assert(noexcept(safe!(1)));
static_assert(!noexcept(risky!(1)));
static_assert(noexcept(quiet!(1)));

template <class T> constexpr bool nt(T t) { return noexcept(risky!(t)); }
template <class T> constexpr bool nq(T t) { return noexcept(quiet!(t)); }
static_assert(!nt(1));
static_assert(nq(1));

void use() {
  // The expansion has an effect (a call), so a discarded invocation is not
  // an unused value.
  quiet!(1);
  // A pure expansion is not diagnosed either: an invocation is conservatively
  // treated as having side effects.
  safe!(1);
}

// A statement-position invocation in a template leaves no empty expression
// statement behind at instantiation.
__macro let(std::meta::token_sequence name, int init) {
  return ^^{ auto \(name) = \(init) };
}
template <int N> int d() {
  let!(x, N * 2);
  return x + 1;
}
int use_d() { return d<2>(); }
