// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify=expected,ref %s
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify=expected,both -fexperimental-new-constant-interpreter %s

// Copying a class with an anonymous union member whose member was activated by assignment, the copy of a union array
// member, and the elements of an array member that are alive with an erroneous value (#305).

struct S1 { union { int x; int y; }; };
constexpr S1 g() { S1 s; s.x = 1; return s; }
constexpr S1 r1 = g();
static_assert(r1.x == 1);

struct SP { int p; union { int x; int y; }; };
constexpr int c() { SP s; s.p = 0; s.x = 1; SP t = s; return t.x + t.p; }
static_assert(c() == 1);

struct W { union { int a[2]; int z; }; };
constexpr W h() { W w; w.a[0] = 1; w.a[1] = 2; return w; }
constexpr W okw = h();
static_assert(okw.a[0] + okw.a[1] == 3);

// The array stays the active member in the copy.
union U { int arr[3]; int x; };
constexpr U mk() { U u; __builtin_start_lifetime(&u.arr); u.x = 0; u.arr[0] = 4; u.arr[1] = 5; u.arr[2] = 6; U v = u; return v; }
constexpr U mu = mk();
static_assert(mu.arr[0] == 4);

// Elements that are alive (the array was created by assigning to one of them) have an erroneous value unless assigned.
union Ind { int arr[4]; int x; }; // #ind-decl
constexpr Ind f() { Ind u; u.arr[1] = 3; return u; }
constexpr Ind ind = f(); // expected-error {{must be initialized by a constant expression}} \
                         // ref-note {{subobject 'arr' is not initialized}} \
                         // both-note {{subobject 'arr' is not initialized}} \
                         // expected-note@#ind-decl {{subobject declared here}}

// start_lifetime starts the lifetime of no element: the elements may be left out of the value.
constexpr Ind g2() { Ind u; __builtin_start_lifetime(&u.arr); return u; }
constexpr Ind ok2 = g2();
