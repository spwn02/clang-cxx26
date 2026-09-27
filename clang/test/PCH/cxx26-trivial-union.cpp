// RUN: %clang_cc1 -std=c++26 -emit-pch -o %t %s
// RUN: %clang_cc1 -std=c++26 -include-pch %t -verify %s

// P3074R7: triviality of union special members and the deleted destructor
// survive a PCH round trip.

#ifndef HEADER
#define HEADER

struct S { S() {} ~S() {} int x; };
union Trivial { S s; int i; };
union DeletedDtor { S s = S(); };
union UserCtor { UserCtor() {} S s; };
struct HasAnon { union { S s; int i[2]; }; };

constexpr int f() {
  union { int a[2]; };
  a[0] = 1;
  return a[0];
}

#else

static_assert(__is_trivially_constructible(Trivial) && __is_trivially_destructible(Trivial));
static_assert(!__is_destructible(DeletedDtor));
static_assert(!__is_destructible(UserCtor));
static_assert(__is_trivially_destructible(HasAnon));
static_assert(f() == 1);

Trivial t;
DeletedDtor d; // expected-error {{attempt to use a deleted function}}
// expected-note@12 {{destructor of 'DeletedDtor' is implicitly deleted because variant field 's' has a non-trivial destructor}}

#endif
