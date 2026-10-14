// RUN: %clang_cc1 -std=c++20 -pedantic-errors -verify %s
// RUN: %clang_cc1 -std=c++23 -pedantic-errors -verify %s
// RUN: %clang_cc1 -std=c++2c -pedantic-errors -verify %s
// expected-no-diagnostics

namespace cwg3046 {
// [class.mem]/[basic.types]: layout-compatible enumerations have the same
// underlying type and the same values; "the values of the enumeration" is the
// term of [dcl.enum].

// A fixed underlying type: the values are all values of that type, whatever the
// enumerators are.
enum E1 : int { a = 1, b = 2 };
enum E2 : int { c = 2, d = 1 };
enum E3 : int { e = 1, f = 3 };
enum E4 : int {};
enum E5 : unsigned { j = 1, k = 2 };
static_assert(__is_layout_compatible(E1, E2));
static_assert(__is_layout_compatible(E1, E3));
static_assert(__is_layout_compatible(E1, E4));
static_assert(!__is_layout_compatible(E1, E5));

// No fixed underlying type: the values are those of the hypothetical integer
// type of minimal width that represents all enumerators.
enum N1 { p = 0, q = 1 };                  // 1 bit, unsigned
enum N2 { r = 1, s = 0 };                  // same values as N1
enum N3 { t = 0, u = 255 };                // 8 bits, unsigned
enum N4 { v = -1, w = 0 };                 // 2 bits, signed: [-2, 1]
enum N5 { x = -2, y = 1 };                 // same values as N4
enum N6 { z = -3, z2 = 0 };                // 3 bits, signed: [-4, 3]
static_assert(__is_layout_compatible(N1, N2));
static_assert(!__is_layout_compatible(N1, N3));
static_assert(__is_layout_compatible(N4, N5));
static_assert(!__is_layout_compatible(N4, N6));

struct A { E1 value; int tail; };
struct B { E3 value; int tail; };
struct C { N1 value; int tail; };
struct D { N3 value; int tail; };
static_assert(__is_layout_compatible(A, B));
static_assert(!__is_layout_compatible(C, D));
static_assert(__builtin_is_corresponding_member(&A::tail, &B::tail));
static_assert(!__builtin_is_corresponding_member(&C::tail, &D::tail));
} // namespace cwg3046
