// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify -verify-ignore-unexpected=note %s
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify -verify-ignore-unexpected=note -fexperimental-new-constant-interpreter %s

// Reading a member through a pointer from the destructor of a base class: the member is destroyed before the
// destructors of the bases run, so the read is not a constant expression (#310).
struct M2 { int v; constexpr ~M2() {} };
struct B { int *out; const M2 *mp; constexpr ~B() { *out = mp->v; } };
struct D : B { M2 m; constexpr D(int *o) : B{o, &m}, m{1} {} };
constexpr int f() { int r = 0; { D d(&r); } return r; }
constexpr int r = f(); // expected-error {{must be initialized by a constant expression}}
