// RUN: %clang_cc1 -std=c++2d -fsyntax-only -verify %s

// P3670R4: pack indexing for template names ([temp.names]): TT...[N]<args>.

template <class T> struct A { using type = T; };
template <class T> struct B { using type = T *; };
template <class T> constexpr int vA = 1;
template <class T> constexpr int vB = 2;

template <template <typename> typename... TT>
struct S {
  template <typename T> using First = TT...[0]<T>;
  template <typename T> using Second = TT...[1]<T>;
  template <typename T>
  static constexpr bool second_is_pointer =
      __is_same(typename TT...[1]<T>::type, T *);
};
static_assert(__is_same(S<A, B>::First<int>, A<int>));
static_assert(__is_same(S<A, B>::Second<int>, B<int>));
static_assert(S<A, B>::second_is_pointer<int>);

// A pack of variable templates.
template <template <typename> auto... V>
constexpr int pick = V...[1]<int>;
static_assert(pick<vA, vB> == 2);

// As the type of a member, with a dependent index.
template <template <typename> typename... TT>
struct Mk {
  typename TT...[0]<int>::type member;
  template <unsigned I> using At = TT...[I]<char>;
};
Mk<A, B> m;
static_assert(__is_same(decltype(m.member), int));
static_assert(__is_same(Mk<A, B>::At<1>, B<char>));

// The index must be in range.
template <template <typename> typename... TT>
using Bad = TT...[2]<int>; // expected-error {{invalid index 2 for pack 'TT' of size 2}}
using X = Bad<A, B>;       // expected-note {{in instantiation of template type alias 'Bad' requested here}}

// The name before '...[' must denote a pack.
template <template <typename> typename T>
struct NotAPack {
  template <typename U> using I = T...[0]<U>; // expected-error {{'T' does not refer to the name of a parameter pack}}
};

// Indexing a pack of concepts as a type-constraint.
template <class T> concept C1 = true;
template <class T> concept C2 = __is_same(T, int);
template <template <class> concept... CC>
struct Concepts {
  template <CC...[0] T> void f(T);
  template <CC...[1] T> void g(T);
  void h(CC...[1] auto);
};
Concepts<C1, C2> cs;
void use_concepts() {
  cs.f(1);
  cs.f(1.0);
  cs.g(1);
  cs.g(1.0); // expected-error {{no matching member function for call to 'g'}}
  cs.h(1);
  cs.h(1.0); // expected-error {{no matching member function for call to 'h'}}
}
// expected-note@54 {{candidate template ignored: constraints not satisfied [with T = double]}}
// expected-note@54 {{because 'double' does not satisfy 'C2'}}
// expected-note@50 2{{because '__is_same(double, int)' evaluated to false}}
// expected-note@55 {{candidate template ignored: constraints not satisfied [with auto:1 = double]}}
// expected-note@55 {{because 'double' does not satisfy 'C2'}}

// Other places a constrained placeholder or a concept-id can appear.
namespace concept_pack_uses {
template <class T> concept Any = true;
template <class T> concept IsInt = __is_same(T, int);
template <template <class> concept... CC>
struct S {
  void local() { CC...[0] auto v = 1; (void)v; }          // deduced variable
  CC...[1] auto ret() { return 1; }                        // deduced return type
  template <CC...[0] auto N> void nttp() {}                // placeholder NTTP
  template <class T>
  static constexpr bool rtr0 = requires(T t) { { t } -> CC...[0]; }; // return-type-requirement
  template <class T>
  static constexpr bool rtr1 = requires(T t) { { t } -> CC...[1]; };
};
template struct S<Any, IsInt>;
static_assert(S<Any, IsInt>::rtr0<int>);
// The constraint applies to decltype((t)), which is 'int &'.
static_assert(!S<Any, IsInt>::rtr1<int>);
int use() {
  S<Any, IsInt>().nttp<1>();
  return S<Any, IsInt>().ret();
}
}

// Out-of-line definitions match their declarations when the constraint names
// CC...[N] (the substituted name is canonically the selected element).
namespace out_of_line {
template <class T> concept Any = true;
template <template <class> concept... CC>
struct S {
  template <CC...[0] T> void f(T);
  template <class T> requires CC...[0]<T> void g(T);
  void h(CC...[0] auto);
};
template <template <class> concept... CC>
template <CC...[0] T> void S<CC...>::f(T) {}
template <template <class> concept... CC>
template <class T> requires CC...[0]<T> void S<CC...>::g(T) {}
template <template <class> concept... CC>
void S<CC...>::h(CC...[0] auto) {}
void use() { S<Any>().f(1); S<Any>().g(1); S<Any>().h(1); }
}

// A type-constraint may be expanded over a type template parameter pack.
template <class T> concept Any = true;
template <template <class> concept... CC>
struct ExpandedConcepts {
  template <CC... Ts> struct Expanded {};
};

namespace in_expression {
// Parsing regression: a pack-index-template-name used as the head of an
// expression ('T...[N]<args>{}', '()' or '::name') asserted in the parser.
template <typename T> struct A { static const int value = 1; };
template <typename T> struct B { static const int value = 2; };
template <template <typename> class... TT>
int f() {
  auto a = TT...[1]<int>{};
  auto b = TT...[0]<int>();
  return a.value + b.value + TT...[1]<int>::value;
}
template int f<A, B>();
static_assert(B<int>::value == 2);
}
