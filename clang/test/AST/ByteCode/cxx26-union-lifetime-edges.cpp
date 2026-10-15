// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify -verify-ignore-unexpected=note %s
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify -verify-ignore-unexpected=note -fexperimental-new-constant-interpreter %s

// P3726R2 [class.union.general], [obj.lifetime]: std::start_lifetime starts the lifetime of the array and of no element;
// edges of the lifetime state of the elements of an array member of a union (#305). Both evaluators agree; the notes
// differ and are not checked.

inline void *operator new(__SIZE_TYPE__, void *p) noexcept { return p; }
inline void *operator new[](__SIZE_TYPE__, void *p) noexcept { return p; }

// Elements of class type.
struct T { int x; };
union C { T t[2]; int z; };
constexpr int a1() { C u; __builtin_start_lifetime(&u.t); u.t[0].x = 5; return 0; }
constexpr int ra1 = a1(); // expected-error {{must be initialized by a constant expression}}
// Another member ending the array's lifetime and an assignment creating it again: the elements are alive.
constexpr int a2() { C u; __builtin_start_lifetime(&u.t); u.z = 1; u.t[0].x = 5; u.t[1].x = 6; return u.t[0].x + u.t[1].x; }
static_assert(a2() == 11);
consteval bool a3() {
  C u;
  __builtin_start_lifetime(&u.t);
  bool before = __builtin_is_within_lifetime(&u.t[0]);
  new (&u.t[0]) T();
  return !before && __builtin_is_within_lifetime(&u.t[0]);
}
static_assert(a3());

// Constructing an element of an array that is a member of a member of a union does not activate the inactive member.
union V { struct P { int a[2]; } s; int x; };
constexpr int b1() { V v{.x = 1}; new (&v.s.a[0]) int(3); return v.s.a[0]; }
constexpr int rb1 = b1(); // expected-error {{must be initialized by a constant expression}}

// Constructing the array after start_lifetime creates its elements: they are alive.
union U { int arr[3]; int x; };
constexpr U c1() { U u; __builtin_start_lifetime(&u.arr); new (&u.arr) int[3]; return u; }
constexpr U rc1 = c1(); // expected-error {{must be initialized by a constant expression}}
constexpr U c2() { U u; __builtin_start_lifetime(&u.arr); new (&u.arr) int[3]{}; return u; }
constexpr U rc2 = c2();

// Assigning a union copies the state of its elements.
constexpr int d1() {
  U a; a.arr[0] = 1; a.arr[1] = 2; a.arr[2] = 3;
  U b; __builtin_start_lifetime(&b.arr); new (&b.arr[0]) int(9);
  a = b;
  return a.arr[1];
}
constexpr int rd1 = d1(); // expected-error {{must be initialized by a constant expression}}
constexpr U d2() { U a; __builtin_start_lifetime(&a.arr); U b; b.arr[0] = 1; a = b; return a; }
constexpr U rd2 = d2(); // expected-error {{must be initialized by a constant expression}}

// The copy of an array member of a class that is not a union keeps its uninitialized elements.
struct NU { int a[3]; };
constexpr NU nu() { NU s; s.a[0] = 1; return s; }
constexpr NU rnu = nu(); // expected-error {{must be initialized by a constant expression}}
