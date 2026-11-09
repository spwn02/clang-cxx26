// RUN: %clang_cc1 -std=c++2d -fsyntax-only -verify %s

// P3822R2: a conditional noexcept in a compound requirement:
// { E } noexcept(constant-expression). If the value is true, E shall not be a
// potentially-throwing expression; 'noexcept(false)' imposes nothing.

void throws();
void nothrows() noexcept;

static_assert(requires { { nothrows() } noexcept(true); });
static_assert(requires { { throws() } noexcept(false); });
static_assert(!requires { { throws() } noexcept(true); });
static_assert(requires { { throws() } noexcept(1 > 2); });
static_assert(requires { { nothrows() } noexcept; });
static_assert(!requires { { throws() } noexcept; });

template <class T> concept CT = requires(T t) { { t.f() } noexcept(true); };
template <class T> concept CF = requires(T t) { { t.f() } noexcept(false); };
template <class T> concept Cond = requires(T t) { { t.f() } noexcept(T::flag); };
template <class T> concept Dep = requires(T t) { { t.f() } noexcept(noexcept(t.f())); };
template <class T, bool B> concept Param = requires(T t) { { t.f() } noexcept(B); };
template <class T, bool... Bs> concept Pack = (requires(T t) { { t.f() } noexcept(Bs); } && ...);

struct Th { void f(); static constexpr bool flag = true; };
struct No { void f() noexcept; static constexpr bool flag = true; };
struct ThF { void f(); static constexpr bool flag = false; };

static_assert(!CT<Th> && CT<No>);
static_assert(CF<Th> && CF<No>);
static_assert(!Cond<Th> && Cond<No> && Cond<ThF>);
static_assert(Dep<Th> && Dep<No>); // noexcept(noexcept(t.f())) holds the same
static_assert(Param<Th, false> && !Param<Th, true> && Param<No, true>);
static_assert(Pack<Th, false, false> && !Pack<Th, false, true>);

// A substitution failure in the condition makes the requirement unsatisfied.
template <class T> concept Bad = requires(T t) { { t.f() } noexcept(T::missing); };
static_assert(!Bad<Th>);

// The condition precedes the return-type-requirement.
template <class A, class B> concept Same = __is_same(A, B);
template <class T, bool B> concept WithRet = requires(T t) { { t.f() } noexcept(B) -> Same<int>; };
struct R { int f(); };
struct Rn { int f() noexcept; };
struct Rl { long f() noexcept; };
static_assert(WithRet<R, false> && !WithRet<R, true>);
static_assert(WithRet<Rn, true> && !WithRet<Rl, true>);

// The condition is a contextually converted constant expression of type bool.
void use(int n) { // expected-note {{declared here}}
  (void)requires { { throws() } noexcept(n); }; // expected-error {{noexcept specifier argument is not a constant expression}} \
                                                // expected-note {{function parameter 'n' with unknown value cannot be used in a constant expression}}
}
