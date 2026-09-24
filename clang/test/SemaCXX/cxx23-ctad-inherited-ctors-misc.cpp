// RUN: %clang_cc1 -fsyntax-only -std=c++23 -verify %s

template <typename T> struct B { B(T); };
template <typename T> struct C : B<T> { using B<T>::B; };

// An alias template of the derived class uses its inherited guides.
template <typename T> using CA = C<T>;
CA ca(42);
static_assert(__is_same(decltype(ca), C<int>));

// The base is deduced on its own before the derived class is.
B b0(1.5);
static_assert(__is_same(decltype(b0), B<double>));
C c0('x');
static_assert(__is_same(decltype(c0), C<char>));

// Explicit constructors are not candidates for copy-initialization.
template <typename T> struct X { // expected-note {{inherited from implicit deduction guide declared here}} \
                                 // expected-note {{implicit deduction guide declared as 'template <typename T> X(X<T>) -> X<T>'}}
  explicit X(T); // expected-note {{inherited from implicit deduction guide declared here}} \
                 // expected-note {{implicit deduction guide declared as 'template <typename T> explicit X(T) -> X<T>'}}
};
template <typename T> struct Y : X<T> { // expected-note {{candidate template ignored: could not match 'Y<T>' against 'int'}} \
                                        // expected-note {{candidate template ignored: could not match 'X<T>' against 'int'}} \
                                        // expected-note 2{{implicit deduction guide declared as}}
  using X<T>::X; // expected-note {{candidate template ignored: could not match 'X<T>' against 'int'}} \
                 // expected-note {{explicit constructor is not a candidate}}
};
Y y1(1);
static_assert(__is_same(decltype(y1), Y<int>));
Y y2 = 1; // expected-error {{no viable constructor or deduction guide for deduction of template arguments of 'Y'}}

// A guide for the base declared between two uses of the derived class.
template <typename T> struct P { P(T, T); };
template <typename T> struct Q : P<T> { using P<T>::P; };
Q q1(1, 2);
static_assert(__is_same(decltype(q1), Q<int>));
P(long, long) -> P<long>;
Q q2(1L, 2L);
static_assert(__is_same(decltype(q2), Q<long>));
Q q3(1, 2);
static_assert(__is_same(decltype(q3), Q<int>));

// The derived class has constructors of its own as well.
template <typename T> struct R : B<T> {
  using B<T>::B;
  R(T, T);
};
R r1(1);
R r2(1, 2);
static_assert(__is_same(decltype(r1), R<int>));
static_assert(__is_same(decltype(r2), R<int>));

// Inheriting constructors through more than one level.
template <typename T> struct S : C<T> { using C<T>::C; };
S s1(3);
static_assert(__is_same(decltype(s1), S<int>));
