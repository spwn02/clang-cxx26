// RUN: %clang_cc1 -std=c++26 -Wno-vla-cxx-extension -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fexperimental-new-constant-interpreter -Wno-vla-cxx-extension -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++26 -freflection -Wno-vla-cxx-extension -fsyntax-only -verify %s
// RUN: %clang_cc1 -triple x86_64-pc-windows-msvc -std=c++26 -Wno-vla-cxx-extension -fsyntax-only -verify %s

#include "Inputs/std-compare.h"

constexpr int sign(std::strong_ordering o) { return o < 0 ? -1 : o > 0 ? 1 : 0; }

static_assert(__has_builtin(__builtin_type_order));
// As of GCC, the result is a std::strong_ordering (libstdc++'s std::type_order
// initializes one with it).
static_assert(__is_same(decltype(__builtin_type_order(int, char)), std::strong_ordering));
static_assert(sign(__builtin_type_order(int, int)) == 0);
static_assert(__builtin_type_order(int, int) == 0);
static_assert(__builtin_type_order(int, char) != 0);
static_assert(sign(__builtin_type_order(int, char)) == -sign(__builtin_type_order(char, int)));
static_assert(__builtin_type_order(const int, int) != 0);
static_assert(__builtin_type_order(int&, int&&) != 0);
struct Inc;
static_assert(__builtin_type_order(Inc, int) != 0);
static_assert(__builtin_type_order(void, void) == 0);
static_assert(__builtin_type_order(void(), void(*)()) != 0);
using Alias = int;
static_assert(__builtin_type_order(Alias, int) == 0);

template<class T, class U> constexpr int order = sign(__builtin_type_order(T, U));
// The type of a dependent use is known at the definition of the template.
template<class T, class U> struct Holder {
  static constexpr std::strong_ordering value = __builtin_type_order(T, U);
};
template<class T, class U> constexpr std::strong_ordering var = __builtin_type_order(T, U);
static_assert(Holder<int, int>::value == 0);
static_assert(sign(Holder<int, char>::value) == -sign(Holder<char, int>::value));
static_assert(sign(var<int, char>) == sign(Holder<int, char>::value));
static_assert(order<Inc, int> == sign(__builtin_type_order(Inc, int)));
template<int> struct Value {};
Value<sign(__builtin_type_order(char, int))> value;

using V = int __attribute__((vector_size(16)));
using E = int __attribute__((ext_vector_type(4)));
static_assert(!__is_same(V, E));
static_assert(order<V, E> != 0);
static_assert(order<V, E> == -order<E, V>);
static_assert(order<V*, E*> == -order<E*, V*>);
using F = void();
typedef void NR() __attribute__((noreturn));
static_assert(!__is_same(F, NR));
static_assert(order<F, NR> == -order<NR, F>);
void sized(int* const p __attribute__((pass_object_size(0))));
void unsized(int* const p);
static_assert(!__is_same(decltype(sized), decltype(unsized)));
static_assert(order<decltype(sized), decltype(unsized)> ==
              -order<decltype(unsized), decltype(sized)>);

void local_types() {
  using L1 = decltype([] {});
  using L2 = decltype([] {});
  using L3 = decltype([] {});
  static_assert(order<L1, L2> != 0);
  static_assert(order<L1, L2> == -order<L2, L1>);
  static_assert(!(order<L1, L2> < 0 && order<L2, L3> < 0) || order<L1, L3> < 0);
  using U1 = struct {};
  using U2 = struct {};
  static_assert(order<U1, U2> != 0);
  static_assert(order<U1, U2> == -order<U2, U1>);
  struct A {};
  using Outer = A;
  {
    struct A {};
    static_assert(order<Outer, A> != 0);
    static_assert(order<Outer, A> == -order<A, Outer>);
  }
}

auto bad1 = __builtin_type_order(int); // expected-error {{type trait requires 2 arguments; have 1 argument}}
auto bad3 = __builtin_type_order(int, int, int); // expected-error {{type trait requires 2 arguments; have 3 arguments}}
auto not_type = __builtin_type_order(42, int); // expected-error {{expected a type}}
void vla(int n) {
  using V = int[n];
  (void)__builtin_type_order(V, int); // expected-error {{variable length arrays are not supported in '__builtin_type_order'}}
  (void)__builtin_type_order(V*, int); // expected-error {{variable length arrays are not supported in '__builtin_type_order'}}
}
