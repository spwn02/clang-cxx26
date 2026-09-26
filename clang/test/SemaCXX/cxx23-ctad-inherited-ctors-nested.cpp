// RUN: %clang_cc1 -fsyntax-only -std=c++23 -verify %s

// P2582R1: deduction guides from the inherited constructors of a member template
// of a class template (and of an explicit specialization of a member template)
// (spwn02/clang-cxx26#122). The template parameters of the enclosing templates
// must not be confused with the ones of the member template, so all of the types
// below are deliberately distinct.

template <typename T> struct Base {
  Base(T);
};

template <typename A, typename B> struct Pair {
  Pair(A, B);
};

template <typename T> struct Outer {
  // U cannot be deduced, which is an error.
  template <typename U> struct OuterBase : Base<T> { // #OuterBase
    using Base<T>::Base;
  };
  template <typename U> struct InnerBase : Base<U> {
    using Base<U>::Base;
  };
  // The parameters of the base are the member template's, swapped.
  template <typename U, typename V> struct Swapped : Pair<V, U> {
    using Pair<V, U>::Pair;
  };
  // A base that mixes the enclosing template's parameter with the member's.
  template <typename U> struct Mixed : Pair<T, U> {
    using Pair<T, U>::Pair;
  };
  // The base is named through a member typedef of the enclosing template; U
  // cannot be deduced.
  using B = Base<T>;
  template <typename U> struct ViaTypedef : B { // #ViaTypedef
    using B::B;
  };
  // A default argument covers the parameter that the base does not mention;
  // a pack deduces to the empty pack.
  template <typename U = long> struct WithDefault : Base<T> {
    using Base<T>::Base;
  };
  template <typename... Us> struct Pack : Base<T> {
    using Base<T>::Base;
  };
  // More than one level of enclosing templates.
  template <typename U> struct Mid {
    template <typename V> struct In : Pair<U, V> {
      using Pair<U, V>::Pair;
    };
  };
};

Outer<int>::OuterBase a(10); // expected-error {{no viable constructor or deduction guide for deduction of template arguments of 'Outer<int>::OuterBase'}}
// expected-note@#OuterBase {{candidate template ignored: couldn't infer template argument 'U'}}
// expected-note@#OuterBase {{implicit deduction guide declared as 'template <typename U> OuterBase(Base<int>) -> Outer<int>::OuterBase<U>'}}
// expected-note@#OuterBase {{candidate template ignored: could not match 'Outer<int>::OuterBase<U>' against 'int'}}
// expected-note@#OuterBase {{implicit deduction guide declared as 'template <typename U> OuterBase(Outer<int>::OuterBase<U>) -> Outer<int>::OuterBase<U>'}}
// expected-note@#OuterBase {{candidate function template not viable: requires 0 arguments, but 1 was provided}}
// expected-note@#OuterBase {{implicit deduction guide declared as 'template <typename U> OuterBase() -> Outer<int>::OuterBase<U>'}}

Outer<int>::ViaTypedef v(1); // expected-error {{no viable constructor or deduction guide for deduction of template arguments of 'Outer<int>::ViaTypedef'}}
// expected-note@#ViaTypedef 0+ {{candidate}}
// expected-note@#ViaTypedef 0+ {{implicit deduction guide declared as}}

Outer<int>::WithDefault w(2);
static_assert(__is_same(decltype(w), Outer<int>::WithDefault<long>));

Outer<int>::Pack p(3);
static_assert(__is_same(decltype(p), Outer<int>::Pack<>));

Outer<int>::InnerBase b('c');
static_assert(__is_same(decltype(b), Outer<int>::InnerBase<char>));

Outer<int>::Swapped c('a', 1.5);
static_assert(__is_same(decltype(c), Outer<int>::Swapped<double, char>));

Outer<int>::Mixed d(1, 'x');
static_assert(__is_same(decltype(d), Outer<int>::Mixed<char>));

Outer<long>::InnerBase e(1.5f);
static_assert(__is_same(decltype(e), Outer<long>::InnerBase<float>));

Outer<int>::Mid<char>::In f('a', 2.5);
static_assert(__is_same(decltype(f), Outer<int>::Mid<char>::In<double>));

// The same holds for an explicit specialization of a member template.
template <typename T> struct Outer2 {
  template <typename U> struct Inner : Base<U> {
    using Base<U>::Base;
  };
};
template <> template <typename U> struct Outer2<int>::Inner : Base<U> {
  using Base<U>::Base;
};

Outer2<int>::Inner g('c');
static_assert(__is_same(decltype(g), Outer2<int>::Inner<char>));

// A member template of a member of a specialization, deduced from a copy.
Outer<int>::InnerBase h(b);
static_assert(__is_same(decltype(h), Outer<int>::InnerBase<char>));
