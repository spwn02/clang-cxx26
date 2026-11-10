// RUN: %clang_cc1 -std=c++2d -fsyntax-only -verify %s

// P2287R6: designated initializers for base classes ([dcl.init.aggr]).

namespace draft_examples {
struct A { int a1, a2; };
struct B : A { int b; };
struct C : A { int a1; };

A v0 = A{1, .a2=2};           // expected-error {{in an initializer list with designators, only the initializers of base classes can be positional}}
constexpr B v1 = B{.a1=1, .b=2};
constexpr B v2 = B{.a1=1, .a2=2, .b=3};
constexpr B v3 = B{A{1, 2}, .b=3};
B v4 = B{A{}, .a2=1, .b=3};   // expected-error {{base class 'A' is initialized both by a positional initializer and by designated initializers}}
constexpr C v5 = C{.a1=4};

static_assert(v1.a1 == 1 && v1.a2 == 0 && v1.b == 2);
static_assert(v2.a1 == 1 && v2.a2 == 2 && v2.b == 3);
static_assert(v3.a1 == 1 && v3.a2 == 2 && v3.b == 3);
static_assert(v5.a1 == 4 && v5.A::a1 == 0 && v5.a2 == 0); // C::a1 hides A::a1

struct A1 { int a; };
struct B1 : A1 { int b; };
struct C1 : B1 { int c; };
constexpr B1 x = B1{.a=1};
constexpr C1 y = C1{.a=2, .b=3, .c=4};
static_assert(x.a == 1 && x.b == 0);
static_assert(y.a == 2 && y.b == 3 && y.c == 4);
} // namespace draft_examples

namespace order_and_lookup {
struct A { int x; int y; int z; };
struct B : A { int q; };
B e{.x = 1, .q = 3};
B f{.q = 3, .x = 1};       // expected-error {{designators naming members of base classes must be specified in declaration order; field 'q' will be initialized after field 'x'}}
B g2{.y = 1, .x = 2};      // expected-error {{field 'y' will be initialized after field 'x'}}

struct C { int p; int x; };
struct D : A, C { };
D g{.y=1, .p=2};
D h{.x=2};                 // expected-error {{designator 'x' is ambiguous: it names members of several base class subobjects of 'D'}}
D h2{.p=1, .y=2};          // expected-error {{field 'p' will be initialized after field 'y'}}

struct NonAggr { int na; NonAggr(int); };
struct E : NonAggr { int e; };
E i{.na=1, .e=2};          // expected-error {{field designator 'na' does not refer to any field in type 'E'}}

// A member of the class hides those of its bases.
struct Hides : A { int x; };
Hides hd{.x = 1};
}

namespace templates {
template <class T> struct Base { T v; };
template <class T> struct Der : Base<T> { T w; };
template <class T> constexpr Der<T> mk(T t) { return Der<T>{.v = t, .w = t + 1}; }
static_assert(mk(3).v == 3 && mk(3).w == 4);
constexpr Der<int> d2{.v = 1, .w = 2};
static_assert(d2.v == 1 && d2.w == 2);
}

namespace nested {
struct A { int x; };
struct B : A { int q; };
struct Outer { B inner; int o; };
constexpr Outer o1{.inner = {.x = 1, .q = 2}, .o = 3};
static_assert(o1.inner.x == 1 && o1.inner.q == 2 && o1.o == 3);
}

namespace default_member_initializers {
struct S { int v = 7; int w = 1; };
struct BS : S { int x; };
constexpr BS bs{.w = 2, .x = 9};
static_assert(bs.v == 7 && bs.w == 2 && bs.x == 9);
}
