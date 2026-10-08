// RUN: %clang_cc1 -std=c++20 -pedantic-errors -verify %s
// RUN: %clang_cc1 -std=c++23 -pedantic-errors -verify %s
// RUN: %clang_cc1 -std=c++2c -pedantic-errors -verify %s
// expected-no-diagnostics

namespace cwg3046 {
enum E1 : int { a = 1, b = 2 };
enum E2 : int { c = 2, d = 1 };
enum E3 : int { e = 1, f = 3 };
enum E4 : int { g = 1, h = 1, i = 2 };
enum E5 : unsigned { j = 1, k = 2 };

static_assert(__is_layout_compatible(E1, E2));
static_assert(__is_layout_compatible(E1, E4));
static_assert(!__is_layout_compatible(E1, E3));
static_assert(!__is_layout_compatible(E1, E5));

struct A { E1 value; int tail; };
struct B { E2 value; int tail; };
struct C { E3 value; int tail; };
static_assert(__is_layout_compatible(A, B));
static_assert(!__is_layout_compatible(A, C));
static_assert(__builtin_is_corresponding_member(&A::tail, &B::tail));
static_assert(!__builtin_is_corresponding_member(&A::tail, &C::tail));
} // namespace cwg3046
