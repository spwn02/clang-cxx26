// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify=expected,ref %s
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify=expected,both -fexperimental-new-constant-interpreter %s

// P3726R2, [class.union.general]: std::start_lifetime begins the lifetime of the array member of a union and of no
// element of it. An element that has not been constructed is outside its lifetime, so assigning to it is not a
// constant expression (the example of the paper that assigns a.arr[2] contradicts its wording, #306).

inline void *operator new(__SIZE_TYPE__, void *p) noexcept { return p; }

union U { int arr[4]; int x; };

constexpr int assign_not_alive() {
  U u;
  __builtin_start_lifetime(&u.arr);
  u.arr[2] = 2; // ref-note {{assignment to object outside its lifetime is not allowed in a constant expression}} \
                // both-note {{assignment to variable whose lifetime has ended}}
  return 0;
}
constexpr int r1 = assign_not_alive(); // expected-error {{must be initialized by a constant expression}} \
                                       // expected-note {{in call to}}

constexpr int assign_after_construct() {
  U u;
  __builtin_start_lifetime(&u.arr);
  new (&u.arr[1]) int(1);
  u.arr[1] = 5; // element 1 is alive
  return u.arr[1];
}
static_assert(assign_after_construct() == 5);

// Assigning to an element of a union whose array is not within its lifetime begins the lifetime of the array.
constexpr int assign_starts_array() {
  U u;
  u.arr[1] = 3;
  u.arr[2] = 4;
  return u.arr[1] + u.arr[2];
}
static_assert(assign_starts_array() == 7);

constexpr int assign_initialized() {
  U u{.arr = {1, 2, 3, 4}};
  u.arr[2] = 9;
  return u.arr[2];
}
static_assert(assign_initialized() == 9);

// Another member ends the lifetime of the array; assigning to an element creates the array again.
constexpr int switch_members() {
  U u;
  __builtin_start_lifetime(&u.arr);
  u.x = 1;
  u.arr[2] = 5;
  return u.arr[2];
}
static_assert(switch_members() == 5);

struct A { union { int i; int arr[4]; }; };
constexpr A v1; // expected-error {{default initialization of an object of const type 'const A' without a user-provided default constructor}}
