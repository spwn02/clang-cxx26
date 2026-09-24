// RUN: %clang_cc1 -fsyntax-only -std=c++23 -verify %s

// FIXME(spwn02/clang-cxx26#122): Deduction guides are not generated from the
// inherited constructors of a member template of a class template (or of a
// specialization of one). P2582R1 requires them, so the valid code below is
// rejected. What must never happen is that such a template is deduced
// *incorrectly*: the template parameters of the enclosing template must not be
// confused with the ones of the member template. All of the types below are
// deliberately distinct.

template <typename T> struct Base {
  Base(T);
};

template <typename T> struct Outer {
  template <typename U> struct OuterBase : Base<T> { // #OuterBase
    using Base<T>::Base;
  };
  template <typename U> struct InnerBase : Base<U> { // #InnerBase
    using Base<U>::Base;
  };
  template <typename U, typename V> struct Mixed : Base<V> { // #Mixed
    using Base<V>::Base;
  };
};

// U cannot be deduced, which is an error with or without the FIXME.
Outer<int>::OuterBase a(10); // expected-error {{no viable constructor or deduction guide for deduction of template arguments of 'Outer<int>::OuterBase'}}
// expected-note@#OuterBase {{candidate template ignored: couldn't infer template argument 'U'}}
// expected-note@#OuterBase {{implicit deduction guide declared as 'template <typename U> OuterBase(Base<int>) -> Outer<int>::OuterBase<U>'}}
// expected-note@#OuterBase {{candidate template ignored: could not match 'Outer<int>::OuterBase<U>' against 'int'}}
// expected-note@#OuterBase {{implicit deduction guide declared as 'template <typename U> OuterBase(Outer<int>::OuterBase<U>) -> Outer<int>::OuterBase<U>'}}
// expected-note@#OuterBase {{candidate function template not viable: requires 0 arguments, but 1 was provided}}
// expected-note@#OuterBase {{implicit deduction guide declared as 'template <typename U> OuterBase() -> Outer<int>::OuterBase<U>'}}

// This should deduce Outer<int>::InnerBase<char>.
Outer<int>::InnerBase b('c'); // expected-error {{no viable constructor or deduction guide for deduction of template arguments of 'Outer<int>::InnerBase'}}
// expected-note@#InnerBase {{candidate template ignored: could not match 'Outer<int>::InnerBase<U>' against 'char'}}
// expected-note@#InnerBase {{implicit deduction guide declared as 'template <typename U> InnerBase(Outer<int>::InnerBase<U>) -> Outer<int>::InnerBase<U>'}}
// expected-note@#InnerBase {{candidate template ignored: could not match 'Base<U>' against 'char'}}
// expected-note@#InnerBase {{implicit deduction guide declared as 'template <typename U> InnerBase(Base<U>) -> Outer<int>::InnerBase<U>'}}
// expected-note@#InnerBase {{candidate function template not viable: requires 0 arguments, but 1 was provided}}
// expected-note@#InnerBase {{implicit deduction guide declared as 'template <typename U> InnerBase() -> Outer<int>::InnerBase<U>'}}

// This should deduce Outer<int>::Mixed<char, double>.
Outer<int>::Mixed c('a', 1.5); // expected-error {{no viable constructor or deduction guide for deduction of template arguments of 'Outer<int>::Mixed'}}
// expected-note@#Mixed {{candidate function template not viable: requires 1 argument, but 2 were provided}}
// expected-note@#Mixed {{implicit deduction guide declared as 'template <typename U, typename V> Mixed(Outer<int>::Mixed<U, V>) -> Outer<int>::Mixed<U, V>'}}
// expected-note@#Mixed {{candidate function template not viable: requires 0 arguments, but 2 were provided}}
// expected-note@#Mixed {{implicit deduction guide declared as 'template <typename U, typename V> Mixed() -> Outer<int>::Mixed<U, V>'}}

// The same holds for an explicit specialization of a member template.
template <typename T> struct Outer2 {
  template <typename U> struct Inner : Base<U> {
    using Base<U>::Base;
  };
};
template <> template <typename U> struct Outer2<int>::Inner : Base<U> { // #Outer2Inner
  using Base<U>::Base;
};

Outer2<int>::Inner d('c'); // expected-error {{no viable constructor or deduction guide for deduction of template arguments of 'Outer2<int>::Inner'}}
// expected-note@#Outer2Inner {{candidate template ignored: could not match 'Outer2<int>::Inner<U>' against 'char'}}
// expected-note@#Outer2Inner {{implicit deduction guide declared as 'template <typename U> Inner(Outer2<int>::Inner<U>) -> Outer2<int>::Inner<U>'}}
// expected-note@#Outer2Inner {{candidate template ignored: could not match 'Base<U>' against 'char'}}
// expected-note@#Outer2Inner {{implicit deduction guide declared as 'template <typename U> Inner(Base<U>) -> Outer2<int>::Inner<U>'}}
// expected-note@#Outer2Inner {{candidate function template not viable: requires 0 arguments, but 1 was provided}}
// expected-note@#Outer2Inner {{implicit deduction guide declared as 'template <typename U> Inner() -> Outer2<int>::Inner<U>'}}
