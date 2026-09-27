// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++23 -fsyntax-only -verify %s

// P3074R7: trivial unions. The default constructor of a union is trivial
// (absent default member initializers) and its destructor is trivial (absent a
// user-provided default constructor or a variant member with a default member
// initializer that has a non-trivial destructor), whatever the variant members.

#if __cplusplus > 202302L
#  if __cpp_trivial_union != 202502L
#    error "wrong value for __cpp_trivial_union"
#  endif
#else
#  ifdef __cpp_trivial_union
#    error "__cpp_trivial_union must only be defined in C++26"
#  endif
#endif

struct S { S() {} ~S() {} int x; };
struct D { D() = delete; ~D() = delete; };

#define TRIV_CTOR(T) __is_trivially_constructible(T)
#define TRIV_DTOR(T) __is_trivially_destructible(T)

#if __cplusplus > 202302L
#  define CXX26_ASSERT(...) static_assert(__VA_ARGS__)
#  define PRE26_ASSERT(...) static_assert(!(__VA_ARGS__))
#else
#  define CXX26_ASSERT(...) static_assert(!(__VA_ARGS__))
#  define PRE26_ASSERT(...) static_assert(__VA_ARGS__)
#endif

union U1 { S s; int i; };
CXX26_ASSERT(TRIV_CTOR(U1) && TRIV_DTOR(U1));

union U2 { S s = S(); }; // #U2
// default member initializer: non-trivial ctor, deleted dtor
static_assert(!TRIV_CTOR(U2) && !__is_destructible(U2));

union U3 { S s[10]; };
CXX26_ASSERT(TRIV_CTOR(U3) && TRIV_DTOR(U3));

union U4 { S s; U4 *next = nullptr; };
CXX26_ASSERT(__is_destructible(U4) && TRIV_DTOR(U4));
static_assert(!TRIV_CTOR(U4));

union U5 { U5() {} S s; int i; }; // #U5
// user-provided default constructor: dtor stays deleted
static_assert(!__is_destructible(U5));

union U6 { U6() {} int i; float f; };
static_assert(TRIV_DTOR(U6));

union U7 { U7(int) {} S s; }; // no default constructor: dtor stays deleted
static_assert(!__is_destructible(U7));

union U8 { D d; int i; }; // a variant member no longer deletes the union's constructor
CXX26_ASSERT(TRIV_CTOR(U8) && TRIV_DTOR(U8));

#if __cplusplus > 202302L
union U9 { S s; U9() = default; ~U9() = default; };
static_assert(TRIV_CTOR(U9) && TRIV_DTOR(U9));
#endif

union U10 { S s; U10() = delete; };
static_assert(!__is_constructible(U10) && !__is_destructible(U10));

union U11 { S s; ~U11() {} };
static_assert(!TRIV_DTOR(U11));

union U12 { const int a; const S s; }; // all variant members const
static_assert(!__is_constructible(U12));

template <class T> union W { T t; int i; };
CXX26_ASSERT(TRIV_CTOR(W<S>) && TRIV_DTOR(W<S>));

// Copy and move operations are unchanged: still deleted for non-trivial members.
struct NT { NT(const NT &); NT &operator=(const NT &); ~NT(); };
union U13 { NT n; };
static_assert(!__is_constructible(U13, const U13 &));
static_assert(!__is_assignable(U13 &, const U13 &));

// Classes containing unions.
struct Z { W<S> w; };
CXX26_ASSERT(TRIV_CTOR(Z) && TRIV_DTOR(Z));
struct Anon { union { S s; int i; }; };
CXX26_ASSERT(TRIV_CTOR(Anon) && TRIV_DTOR(Anon));
struct AnonUser { AnonUser() {} union { S s; int i; }; };
static_assert(!__is_destructible(AnonUser));
struct AnonNSDMI { union { S s = S(); int i; }; };
static_assert(!__is_destructible(AnonNSDMI));

#if __cplusplus > 202302L
void use() {
  U1 u;
  U3 *p = new U3;
  delete p;
  Anon a;
  W<S> w;
}
#endif

U2 u2; // expected-error {{attempt to use a deleted function}}
// expected-note@#U2 {{destructor of 'U2' is implicitly deleted because variant field 's' has a non-trivial destructor}}
U5 u5; // expected-error {{attempt to use a deleted function}}
// expected-note@#U5 {{destructor of 'U5' is implicitly deleted because variant field 's' has a non-trivial destructor}}

