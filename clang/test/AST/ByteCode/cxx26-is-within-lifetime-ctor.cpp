// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify -fexperimental-new-constant-interpreter %s


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
  constexpr Target() : Target(1) {} // #tgt1
  constexpr Target(int) { if consteval { ok = __builtin_is_within_lifetime(this); } } // expected-note {{pointer to an object whose lifetime has not yet begun}}
};
constexpr bool target() { Target t; return t.ok; } // #tgt2
constexpr bool tv = target(); // expected-error {{must be initialized by a constant expression}} \
                              // expected-note {{in call to 'target()'}} \
                              // expected-note@#tgt2 {{in call to 'Target()'}} \
                              // expected-note@#tgt1 {{in call to 'Target(1)'}}

struct T {
  bool ok = false;
  constexpr T() { if consteval { ok = __builtin_is_within_lifetime(this); } } // expected-note {{pointer to an object whose lifetime has not yet begun}}
};
constexpr bool g() { T t; return t.ok; } // #g1
constexpr bool b = g(); // expected-error {{must be initialized by a constant expression}} \
                        // expected-note {{in call to 'g()'}} \
                        // expected-note@#g1 {{in call to 'T()'}}

// A member subobject: the complete object is still being initialized.
struct M {
  bool ok = false;
  constexpr M() { if consteval { ok = __builtin_is_within_lifetime(this); } } // expected-note {{pointer to an object whose lifetime has not yet begun}}
};
struct P {
  M m;
  constexpr P() {} // #p1
};
constexpr bool h() { P p; return p.m.ok; } // #h1
constexpr bool hb = h(); // expected-error {{must be initialized by a constant expression}} \
                         // expected-note {{in call to 'h()'}} \
                         // expected-note@#h1 {{in call to 'P()'}} \
                         // expected-note@#p1 {{in call to 'M()'}}

// After the constructor returned the object is within its lifetime.
struct After { constexpr After() {} };
consteval bool after() { After a; return __builtin_is_within_lifetime(&a); }
static_assert(after());
