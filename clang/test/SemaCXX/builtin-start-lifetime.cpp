// RUN: %clang_cc1 -std=c++2c -verify %s

// __builtin_start_lifetime(p) is the compiler support of std::start_lifetime ([obj.lifetime]): if the object p points
// to is not within its lifetime its lifetime begins; no initialization is performed and no subobject begins its
// lifetime; for a member of a union it becomes the active member.

namespace std {
using size_t = decltype(sizeof(0));
template <class T> struct allocator {
  constexpr T* allocate(size_t n) { return static_cast<T*>(::operator new(n * sizeof(T))); }
  constexpr void deallocate(T* p, size_t) { ::operator delete(p); }
};
} // namespace std

constexpr void* operator new(std::size_t, void* p) noexcept { return p; }

struct A {
  int a;
  int b;
};

// The object is already within its lifetime: nothing happens.
constexpr bool already_alive() {
  A a{1, 2};
  __builtin_start_lifetime(&a);
  return a.a == 1 && a.b == 2;
}
static_assert(already_alive());

// Storage whose lifetime has not begun: afterwards the object exists, its members do not.
constexpr bool heap_object() {
  std::allocator<A> alloc;
  A* p = alloc.allocate(1);
  __builtin_start_lifetime(p);
  ::new (static_cast<void*>(&p->a)) int(1);
  ::new (static_cast<void*>(&p->b)) int(2);
  bool ok = p->a == 1 && p->b == 2;
  alloc.deallocate(p, 1);
  return ok;
}
static_assert(heap_object());

constexpr int members_do_not_exist() {
  std::allocator<A> alloc;
  A* p = alloc.allocate(1);
  __builtin_start_lifetime(p);
  int v = p->a; // expected-note {{read of object outside its lifetime is not allowed in a constant expression}}
  alloc.deallocate(p, 1);
  return v;
}
constexpr int use_members_do_not_exist = members_do_not_exist(); // expected-error {{must be initialized by a constant expression}} \
                                                                 // expected-note {{in call to}}

// Arrays are aggregates too.
constexpr bool array_object() {
  std::allocator<int[3]> alloc;
  int(*p)[3] = alloc.allocate(1);
  __builtin_start_lifetime(p);
  ::new (static_cast<void*>(&(*p)[1])) int(7);
  bool ok = (*p)[1] == 7;
  alloc.deallocate(p, 1);
  return ok;
}
static_assert(array_object());

// A member of a union becomes the active member.
union U {
  int i;
  A a;
};

constexpr bool union_member() {
  U u{.i = 1};
  __builtin_start_lifetime(&u.a);
  ::new (static_cast<void*>(&u.a.a)) int(3);
  ::new (static_cast<void*>(&u.a.b)) int(4);
  return u.a.a == 3 && u.a.b == 4;
}
static_assert(union_member());

constexpr int inactive_member_is_gone() {
  U u{.i = 1};
  __builtin_start_lifetime(&u.a);
  return u.i; // expected-note {{read of member 'i' of union with active member 'a' is not allowed in a constant expression}}
}
constexpr int use_inactive_member_is_gone = inactive_member_is_gone(); // expected-error {{must be initialized by a constant expression}} \
                                                                      // expected-note {{in call to}}

constexpr bool null_pointer() {
  A* p = nullptr;
  __builtin_start_lifetime(p); // expected-note {{construction of dereferenced null pointer is not allowed in a constant expression}}
  return true;
}
static_assert(null_pointer()); // expected-error {{static assertion expression is not an integral constant expression}} \
                               // expected-note {{in call to}}

// Argument checking.
void checks(A* p, void* v, int n, void (*f)(), const A* c) {
  __builtin_start_lifetime(p);
  __builtin_start_lifetime(c);
  __builtin_start_lifetime(n);  // expected-error {{non-pointer argument to '__builtin_start_lifetime' is not allowed}}
  __builtin_start_lifetime(v);  // expected-error {{void pointer argument to '__builtin_start_lifetime' is not allowed}}
  __builtin_start_lifetime(f);  // expected-error {{function pointer argument to '__builtin_start_lifetime' is not allowed}}
  __builtin_start_lifetime();   // expected-error {{too few arguments to function call}}
  __builtin_start_lifetime(p, p); // expected-error {{too many arguments to function call}}
}

struct Incomplete; // expected-note {{forward declaration of 'Incomplete'}}
void incomplete(Incomplete* p) {
  __builtin_start_lifetime(p); // expected-error {{incomplete type 'Incomplete' where a complete type is required}}
}
