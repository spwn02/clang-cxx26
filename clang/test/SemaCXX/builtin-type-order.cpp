// RUN: %clang_cc1 -std=c++26 -Wno-vla-cxx-extension -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fexperimental-new-constant-interpreter -Wno-vla-cxx-extension -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++26 -freflection -Wno-vla-cxx-extension -fsyntax-only -verify %s
// RUN: %clang_cc1 -triple x86_64-pc-windows-msvc -std=c++26 -Wno-vla-cxx-extension -fsyntax-only -verify %s

static_assert(__has_builtin(__builtin_type_order));
static_assert(__is_same(decltype(__builtin_type_order(int, char)), int));
static_assert(__builtin_type_order(int, int) == 0);
static_assert(__builtin_type_order(int, char) != 0);
static_assert(__builtin_type_order(int, char) == -__builtin_type_order(char, int));
static_assert(__builtin_type_order(const int, int) != 0);
static_assert(__builtin_type_order(int&, int&&) != 0);
struct Inc;
static_assert(__builtin_type_order(Inc, int) != 0);
static_assert(__builtin_type_order(void, void) == 0);
static_assert(__builtin_type_order(void(), void(*)()) != 0);
using Alias = int;
static_assert(__builtin_type_order(Alias, int) == 0);

template<class T, class U> constexpr int order = __builtin_type_order(T, U);
static_assert(order<Inc, int> == __builtin_type_order(Inc, int));
template<int> struct Value {};
Value<__builtin_type_order(char, int)> value;

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

int bad1 = __builtin_type_order(int); // expected-error {{type trait requires 2 arguments; have 1 argument}}
int bad3 = __builtin_type_order(int, int, int); // expected-error {{type trait requires 2 arguments; have 3 arguments}}
int not_type = __builtin_type_order(42, int); // expected-error {{expected a type}}
void vla(int n) {
  using V = int[n];
  (void)__builtin_type_order(V, int); // expected-error {{variable length arrays are not supported in '__builtin_type_order'}}
  (void)__builtin_type_order(V*, int); // expected-error {{variable length arrays are not supported in '__builtin_type_order'}}
}
