// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify -fexperimental-new-constant-interpreter %s

// P3450R1: std::is_within_lifetime<U>(p) is true only if static_cast<const volatile U*>(p) is a constant
// subexpression. The library passes that cast as the second argument of the builtin.
struct B { int x; };
struct D : B { int y; };
struct B2 { int z; };
struct D2 : B, B2 { };
template <class U = void, class T>
consteval bool iwl(const T* p) noexcept {
  return __builtin_is_within_lifetime(p, static_cast<const volatile U*>(p));
}
// same type / void / base: true
constexpr int i = 0;
static_assert(iwl(&i));
static_assert(iwl<void>(&i));
static_assert(iwl<int>(&i));
consteval bool base_ok() { D d{}; return iwl<B>(&d); }
static_assert(base_ok());
// downcast of an object that really is a D
consteval bool down_ok() { D d{}; const B* b = &d; return iwl<D>(b); }
static_assert(down_ok());
// downcast of a non-D
consteval bool down_bad() { B b{}; return iwl<D>(&b); }
static_assert(!down_bad());
consteval bool down_bad2() { D2 d{}; const B2* b = &d; return iwl<D2>(b); }
static_assert(down_bad2());
consteval bool down_bad3() { B2 b{}; return iwl<D2>(&b); }
static_assert(!down_bad3());
// void* source
consteval bool v_ok() { int a = 0; const void* v = &a; return iwl<int>(v); }
static_assert(v_ok());
consteval bool v_bad() { int a = 0; const void* v = &a; return iwl<char>(v); }
static_assert(!v_bad());
// not within lifetime
consteval bool ended() { union { int a; char c; } u{.c = 1}; return iwl<int>(&u.a); }
static_assert(!ended());

// The second argument has to be a pointer; a third argument is rejected.
consteval bool bad_second(const int *p) { return __builtin_is_within_lifetime(p, 1); } // expected-error {{non-pointer argument to '__builtin_is_within_lifetime' is not allowed}}
consteval bool bad_count(const int *p) { return __builtin_is_within_lifetime(p, p, p); } // expected-error {{too many arguments to function call}}
