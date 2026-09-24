// RUN: %clang_cc1 -fsyntax-only -std=c++23 -verify %s

// The examples of [over.match.class.deduct].

template <typename T> struct B {
  B(T); // expected-note {{candidate template ignored: couldn't infer template argument 'T'}} \
        // expected-note {{implicit deduction guide declared as 'template <typename T> E(int) -> E<T>'}}
};

template <typename T> struct C : public B<T> {
  using B<T>::B;
};

template <typename T> struct D : public B<T> {}; // expected-note {{candidate template ignored: could not match 'D<T>' against 'int'}} \
                                                 // expected-note {{candidate template ignored: could not match 'B<T>' against 'int'}} \
                                                 // expected-note {{candidate function template not viable: requires 0 arguments, but 1 was provided}} \
                                                 // expected-note 3{{implicit deduction guide declared as}}

C c(42); // OK, deduces C<int>
static_assert(__is_same(C<int>, decltype(c)));

D d(42); // expected-error {{no viable constructor or deduction guide for deduction of template arguments of 'D'}}

// A deduction guide declared after the guides of C were first declared is
// inherited as well, and is preferred as it is not implicit.
B(int) -> B<char>;
C c2(42); // OK, deduces C<char>
static_assert(__is_same(C<char>, decltype(c2)));

template <typename T> struct E : public B<int> { // expected-note {{candidate template ignored: could not match 'E<T>' against 'int'}} \
                                                 // expected-note {{implicit deduction guide declared as 'template <typename T> E(E<T>) -> E<T>'}}
  using B<int>::B;
};

E e(42); // expected-error {{no viable constructor or deduction guide for deduction of template arguments of 'E'}}

template <typename T, typename U, typename V> struct F {
  F(T, U, V);
};

template <typename T, typename U> struct G : F<U, T, int> {
  using G::F::F;
};

G g(true, 'a', 1); // OK, deduces G<char, bool>
static_assert(__is_same(G<char, bool>, decltype(g)));
