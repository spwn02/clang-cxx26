// RUN: %clang_cc1 -std=c++20 -fsyntax-only -verify %s

struct A { int a; char b; double c; };
struct B { int x; char y; float z; };
struct Virtual { virtual void f(); int v; };
union U { int i; char c; };
struct D { int first; int second; };
struct Derived : D {};
struct Incomplete; // expected-note {{forward declaration of 'Incomplete'}}

static_assert(__builtin_is_corresponding_member(&A::a, &B::x));
static_assert(__builtin_is_corresponding_member(&A::b, &B::y));
static_assert(!__builtin_is_corresponding_member(&A::c, &B::z));
static_assert(!__builtin_is_corresponding_member(&A::a, &B::y));
static_assert(!__builtin_is_corresponding_member(&Virtual::v, &Virtual::v));
static_assert(!__builtin_is_corresponding_member(&U::i, &U::i));
static_assert(!__builtin_is_corresponding_member(static_cast<int A::*>(nullptr), &B::x));

static_assert(__builtin_is_pointer_interconvertible_with_class(&A::a));
static_assert(!__builtin_is_pointer_interconvertible_with_class(&A::b));
static_assert(__builtin_is_pointer_interconvertible_with_class(&U::c));
static_assert(!__builtin_is_pointer_interconvertible_with_class(&Virtual::v));
static_assert(__builtin_is_pointer_interconvertible_with_class(static_cast<int Derived::*>(&D::first)));
static_assert(!__builtin_is_pointer_interconvertible_with_class(static_cast<int Derived::*>(&D::second)));

// The result is a bool usable in constant expressions and templates.
template <auto M1, auto M2> constexpr bool corresponding = __builtin_is_corresponding_member(M1, M2);
static_assert(corresponding<&A::a, &B::x>);
static_assert(!corresponding<&A::c, &B::z>);

void diagnostics(int i, int A::*mp, void (A::*mf)()) {
  (void)__builtin_is_corresponding_member(i, mp);  // expected-error {{argument 1 to '__builtin_is_corresponding_member' must be a pointer to a non-static data member of a complete class, not 'int'}}
  (void)__builtin_is_corresponding_member(mp);     // expected-error {{too few arguments to function call, expected 2, have 1}}
  (void)__builtin_is_corresponding_member(mp, mf); // expected-error {{argument 2 to '__builtin_is_corresponding_member' must be a pointer to a non-static data member of a complete class, not 'void (A::*)()'}}
  (void)__builtin_is_pointer_interconvertible_with_class(i, i); // expected-error {{too many arguments to function call, expected 1, have 2}}
  (void)__builtin_is_pointer_interconvertible_with_class(mf);   // expected-error {{must be a pointer to a non-static data member of a complete class}}
  int Incomplete::*ip = nullptr;
  (void)__builtin_is_pointer_interconvertible_with_class(ip);   // expected-error {{incomplete type 'Incomplete' where a complete type is required}}
}
