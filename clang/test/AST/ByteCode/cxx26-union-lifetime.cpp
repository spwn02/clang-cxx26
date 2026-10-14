// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify=expected,ref %s
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify=expected,both -fexperimental-new-constant-interpreter %s
// expected-no-diagnostics

// P3726R2: __builtin_start_lifetime (std::start_lifetime) begins the lifetime of an array member of a union, the
// elements of it that are not within their lifetimes have no constituent values, and the default constructor of a
// union begins the lifetime of none of its members.

inline void *operator new(__SIZE_TYPE__, void *p) noexcept { return p; }

union U { int arr[4]; int x; };

constexpr int read() {
  U u;
  __builtin_start_lifetime(&u.arr);
  new (&u.arr[1]) int(7);
  new (&u.arr[2]) int(8);
  return u.arr[1] + u.arr[2];
}
static_assert(read() == 15);

constexpr U make() {
  U u;
  __builtin_start_lifetime(&u.arr);
  new (&u.arr[1]) int(7);
  return u;
}
constexpr U made = make();

// Constructing an element of an array member makes the array the active member.
constexpr int elem() {
  U u{.x = 1};
  __builtin_start_lifetime(&u.arr);
  new (&u.arr[0]) int(3);
  return u.arr[0];
}
static_assert(elem() == 3);

consteval bool none_active() {
  U u;
  return !__builtin_is_within_lifetime(&u.x) && !__builtin_is_within_lifetime(&u.arr);
}
static_assert(none_active());
