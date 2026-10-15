// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify -verify-ignore-unexpected=note %s
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify -verify-ignore-unexpected=note -fexperimental-new-constant-interpreter %s

// Reading a member through a pointer from the destructor of a base class: the member is destroyed before the
// destructors of the bases run, so the read is not a constant expression (#310).
struct M2 { int v; constexpr ~M2() {} };
struct B { int *out; const M2 *mp; constexpr ~B() { *out = mp->v; } };
struct D : B { M2 m; constexpr D(int *o) : B{o, &m}, m{1} {} };
constexpr int f() { int r = 0; { D d(&r); } return r; }
constexpr int r = f(); // expected-error {{must be initialized by a constant expression}}

// The same holds for a member of a class type with a trivial destructor ([basic.life]: the destructor call starts).
struct T { int v; };
struct B3 { int *out; const T *tp; constexpr ~B3() { *out = tp->v; } };
struct D3 : B3 { T t; constexpr D3(int *o) : B3{o, &t}, t{1} {} };
constexpr int g() { int r = 0; { D3 d(&r); } return r; }
constexpr int r3 = g(); // expected-error {{must be initialized by a constant expression}}

// A member of a non-class type has no destructor call and is still readable.
struct B4 { int *out; const int *ip; constexpr ~B4() { *out = *ip; } };
struct D4 : B4 { int i; constexpr D4(int *o) : B4{o, &i}, i(5) {} };
constexpr int h() { int r = 0; { D4 d(&r); } return r; }
static_assert(h() == 5);
