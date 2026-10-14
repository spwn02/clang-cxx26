// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s
// expected-no-diagnostics

// P3726R2 [temp.param]: template arguments of array type are equivalent if their corresponding elements are
// both within their lifetimes and equivalent, or both not within their lifetimes.
inline void *operator new(__SIZE_TYPE__, void *p) noexcept { return p; }
union U { int arr[3]; };
constexpr U mk(int a, bool second) {
  U u; __builtin_start_lifetime(&u.arr);
  new (&u.arr[1]) int(a);
  if (second) new (&u.arr[2]) int(a);
  return u;
}
template <U u> struct X {};
template <class A, class B> constexpr bool same = false;
template <class A> constexpr bool same<A, A> = true;
static_assert(same<X<mk(1, false)>, X<mk(1, false)>>);
static_assert(!same<X<mk(1, false)>, X<mk(2, false)>>);
static_assert(!same<X<mk(1, false)>, X<mk(1, true)>>);   // element 2 within its lifetime in one, not in the other
