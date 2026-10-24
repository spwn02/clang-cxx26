// RUN: %clang_cc1 -std=c++20 -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++2c -fsyntax-only -verify %s

// [temp.names]p3: a '<' following the terminal name of a name in a type-only
// context ([temp.res.general]) starts a template-argument-list, so the
// 'template' keyword is not needed. A nested-name-specifier is excluded.

struct S {
  template <class U> struct X {};
  template <class U> using A = U;
};

// type-requirement (CWG2806)
template <class T> concept C1 = requires { typename T::X<int>; };
static_assert(C1<S>);

// elaborated-type-specifier
template <class T> void elaborated() { struct T::X<int>* p = nullptr; (void)p; }

// new-type-id
template <class T> void new_type() { auto* p = new T::X<int>; delete p; }

// trailing-return-type
template <class T> auto trailing() -> T::X<int>*;

// type-id of static_cast
template <class T> void cast_type() { auto* p = static_cast<T::X<int>*>(nullptr); (void)p; }

// defining-type-id
template <class T> using Alias = T::X<int>;

// decl-specifier of a member-declaration and of a parameter in a member-declaration
template <class T> struct Member {
  T::X<int>* m;
  void f(T::X<int>*);
};

// decl-specifier of a simple-declaration in namespace scope
template <class T> T::X<int>* ns_function();
template <class T> T::X<int>* ns_variable = nullptr;

// default argument of a type-parameter
template <class T, class U = T::X<int>> struct Default {};

// friend-type-specifier
template <class T> struct Friend { friend T::X<int>; };

// parameter-declaration of an out-of-line member whose declarator-id is qualified
template <class T> struct Outer { void g(T::X<int>*); };
template <class T> void Outer<T>::g(T::X<int>*) {}

// A nested-name-specifier is not a type-only context.
template <class T> void nns() { T::X<int>::foo(); }
// expected-error@-1 {{use 'template' keyword to treat 'X' as a dependent template name}}

void use() {
  elaborated<S>();
  new_type<S>();
  cast_type<S>();
  (void)Alias<S>();
  Member<S> m;
  (void)m;
  Default<S> d;
  (void)d;
  Friend<S> f;
  (void)f;
}
