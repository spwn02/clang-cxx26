// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify -fexperimental-new-constant-interpreter %s

// Notes: the call chains and the reason (a pointer to an object whose lifetime has not yet begun).
// expected-note@* 0+ {{}}

// [class.base.init]: the initialization of an object is complete when a non-delegating constructor returns, so the
// lifetime of the object has begun in the body of a delegating constructor whose target constructor returned, and
// has not in the body of a non-delegating constructor ([meta.const.eval] is_within_lifetime).

struct D {
  int x;
  bool ok = false;
  constexpr D() : D(1) { if consteval { ok = __builtin_is_within_lifetime(this); } }
  constexpr D(int v) : x(v) {}
};
constexpr bool f() { D d; return d.ok; }
static_assert(f());

// A chain of delegating constructors.
struct Chain {
  bool ok = false;
  constexpr Chain() : Chain(1) { if consteval { ok = __builtin_is_within_lifetime(this); } }
  constexpr Chain(int) : Chain(1, 2) {}
  constexpr Chain(int, int) {}
};
constexpr bool chain() { Chain c; return c.ok; }
static_assert(chain());

// The target constructor itself runs before the object's initialization is complete.
struct Target {
  bool ok = true;
  constexpr Target() : Target(1) {}
  constexpr Target(int) { if consteval { ok = __builtin_is_within_lifetime(this); } }
};
constexpr bool target() { Target t; return t.ok; }
constexpr bool tv = target(); // expected-error {{must be initialized by a constant expression}}

struct T {
  bool ok = false;
  constexpr T() { if consteval { ok = __builtin_is_within_lifetime(this); } }
};
constexpr bool g() { T t; return t.ok; }
constexpr bool b = g(); // expected-error {{must be initialized by a constant expression}}

// A member subobject: the complete object is still being initialized.
struct M {
  bool ok = false;
  constexpr M() { if consteval { ok = __builtin_is_within_lifetime(this); } }
};
struct P {
  M m;
  constexpr P() {}
};
constexpr bool h() { P p; return p.m.ok; }
constexpr bool hb = h(); // expected-error {{must be initialized by a constant expression}}

// After the constructor returned the object is within its lifetime.
struct After { constexpr After() {} };
consteval bool after() { After a; return __builtin_is_within_lifetime(&a); }
static_assert(after());
