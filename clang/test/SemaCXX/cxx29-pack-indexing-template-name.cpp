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

// Indexing a pack of concepts as a type-constraint is not implemented yet.
template <class T> concept C1 = true;
template <template <class> concept... CC>
struct Concepts {
  template <CC...[0] T> void f(T); // expected-error {{a pack-index-template-name as a type-constraint is not implemented yet}}
};
