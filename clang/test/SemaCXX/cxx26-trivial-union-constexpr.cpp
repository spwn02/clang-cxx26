// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify=expected,ref %s
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify=expected,both -fexperimental-new-constant-interpreter %s

// P3074R7, P3726R2: constant evaluation with trivial unions. The default
// constructor of a union begins the lifetime of none of its members; the
// lifetime of an array member begins with __builtin_start_lifetime (the
// library's std::start_lifetime), and the elements of it that are not within
// their lifetimes are inactive union elemental subobjects, which have no
// constituent values.

namespace std {
template <class T> constexpr T *construct_at(T *p) { return ::new (static_cast<void *>(p)) T(); }
template <class T, class A> constexpr T *construct_at(T *p, A a) { return ::new (static_cast<void *>(p)) T(a); }
template <class T> constexpr void destroy_at(T *p) { p->~T(); }
}
inline void *operator new(__SIZE_TYPE__, void *p) noexcept { return p; }

struct S { constexpr S() {} constexpr ~S() {} int x = 0; };

constexpr int f1() {
  union { int s[4]; };
  __builtin_start_lifetime(&s);
  new (&s[0]) int(1);
  new (&s[1]) int(2);
  new (&s[2]) int(3);
  return s[0] + s[1] + s[2];
}
static_assert(f1() == 6);

struct A { int a, b; };
constexpr int f2() {
  union { A s; };
  __builtin_start_lifetime(&s);
  new (&s.a) int(1);
  new (&s.b) int(2);
  return s.a + s.b;
}
static_assert(f2() == 3);

union V { int i[2]; float f; };
constexpr int f3() { V v; __builtin_start_lifetime(&v.i); new (&v.i[1]) int(5); return v.i[1]; }
static_assert(f3() == 5);
constexpr int f4() { V v; return v.i[0]; } // #f4
constexpr int r4 = f4(); // expected-error {{must be initialized by a constant expression}} \
                         // expected-note@#f4 {{read of member 'i' of union with no active member}} \
                         // expected-note {{in call to 'f4()'}}
// A union starts with no active member, also when the first member has implicit-lifetime type.
consteval bool f4b() { V v; return !__builtin_is_within_lifetime(&v.i) && !__builtin_is_within_lifetime(&v.f); }
static_assert(f4b());
constexpr int f5() { V v; v.f = 1; return (int)v.f; }
static_assert(f5() == 1);

// A union with a non-implicit-lifetime first member starts with no active member.
union U1 { S s; int i; };
constexpr int f6() { U1 u; return 1; }
static_assert(f6() == 1);
constexpr int f7() { U1 u; std::construct_at(&u.s); u.s.x = 5; int r = u.s.x; std::destroy_at(&u.s); return r; }
static_assert(f7() == 5);
constexpr int f8() { U1 u; std::construct_at(&u.i, 7); return u.i; }
static_assert(f8() == 7);
constexpr int f9() { U1 u; return u.i; } // #f9
constexpr int r9 = f9(); // expected-error {{must be initialized by a constant expression}} \
                         // expected-note@#f9 {{read of member 'i' of union with no active member}} \
                         // expected-note {{in call to 'f9()'}}

// Storage for a non-trivial type: the classic use case.
union U3 { S s[3]; };
constexpr int f10() {
  U3 u;
  __builtin_start_lifetime(&u.s);
  std::construct_at(&u.s[0]);
  u.s[0].x = 4;
  int r = u.s[0].x;
  std::destroy_at(&u.s[0]);
  return r;
}
static_assert(f10() == 4);

template <class T> union Storage { T v; };
constexpr int f11() {
  Storage<S> st;
  std::construct_at(&st.v);
  st.v.x = 9;
  int r = st.v.x;
  std::destroy_at(&st.v);
  return r;
}
static_assert(f11() == 9);

// A class with an anonymous union member and a user-provided constructor does
// not begin the lifetime of the variant member.
struct WithCtor {
  constexpr WithCtor() {}
  union { int i[2]; float f; };
};
constexpr int f12() { WithCtor w; return w.i[0]; } // #f12
constexpr int r12 = f12(); // expected-error {{must be initialized by a constant expression}} \
                           // expected-note@#f12 {{read of member 'i' of union with no active member}} \
                           // expected-note {{in call to 'f12()'}}

// The elements of an array member that are not within their lifetimes have no
// constituent values: the result may leave them out. P3726R2's example.
struct Arr {
  struct X { int i; int j; }; // #v4-decl
  struct Y { X x1; X x2; };
  union { int i; int arr[4]; Y y; };
};
constexpr Arr v2{.i = 1};
constexpr Arr v3 = []{
  Arr a;
  __builtin_start_lifetime(&a.arr);
  new (&a.arr[1]) int(1);
  new (&a.arr[2]) int(2);
  return a;
}();
// A union elemental subobject of a class type in an array member is skipped as well.
union Ctor { S s[3]; };
constexpr Ctor c1 = []{
  Ctor u;
  __builtin_start_lifetime(&u.s);
  std::construct_at(&u.s[1]);
  return u;
}();
constexpr Arr v4 = []{ // expected-error {{must be initialized by a constant expression}} \
                       // ref-note {{in call to}} \
                       // both-note {{subobject 'j' is not initialized}}
  Arr a;
  a.y.x1.i = 1;
  a.y.x2.j = 2;
  return a; // ref-note {{subobject 'j' is not initialized}} \
            // ref-note {{in call to 'Arr(a)'}} \
            // expected-note@#v4-decl {{subobject declared here}}
}();

// An element that is within its lifetime holds a constituent value even when it is erroneous: only the elements that
// are not within their lifetimes are left out. Assigning to a member of a default-initialized union creates the whole
// array.
union Ind { int arr[4]; int x; }; // #ind-decl
constexpr Ind ind = []{ // expected-error {{must be initialized by a constant expression}} \
                        // expected-note {{subobject 'arr' is not initialized}} \
                        // expected-note@#ind-decl {{subobject declared here}}
  Ind u;
  u.arr[1] = 3;
  return u;
}();
