// RUN: %clang_cc1 -std=c++17 -fsyntax-only -verify=expected %s
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify=expected %s

// [temp.names]p3: a '<' that follows the terminal name of a typename-specifier or of a
// class-or-decltype (type-only contexts other than a nested-name-specifier) starts a
// template-argument-list; the 'template' keyword is not required there
// (CWG343, CWG1812, CWG1710, CWG314).

template <class T> using A1 = typename T::X<int>;
template <class T> struct W {
  using t1 = typename T::template X<int>;
  using t2 = typename T::X<int>;
};

template <class T> struct B { template <class U> struct N {}; };
template <class T> struct D1 : B<T>::N<int> {};

struct S { template <class U> using X = U; };
A1<S> a;
static_assert(__is_same(W<S>::t1, int) && __is_same(W<S>::t2, int));

// 'template' is still required when the template-id is a component of a nested-name-specifier.
template <class T> using Bad2 = typename T::Y<int>::type; // expected-error {{use 'template' keyword to treat 'Y' as a dependent template name}}
