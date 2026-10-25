// RUN: %clang_cc1 -std=c++98 %s -verify=expected -fexceptions -fcxx-exceptions -pedantic-errors
// RUN: %clang_cc1 -std=c++11 %s -verify=expected -fexceptions -fcxx-exceptions -pedantic-errors
// RUN: %clang_cc1 -std=c++14 %s -verify=expected,since-cxx14 -fexceptions -fcxx-exceptions -pedantic-errors
// RUN: %clang_cc1 -std=c++17 %s -verify=expected,since-cxx14 -fexceptions -fcxx-exceptions -pedantic-errors
// RUN: %clang_cc1 -std=c++20 %s -verify=expected,since-cxx14 -fexceptions -fcxx-exceptions -pedantic-errors
// RUN: %clang_cc1 -std=c++23 %s -verify=expected,since-cxx14 -fexceptions -fcxx-exceptions -pedantic-errors
// RUN: %clang_cc1 -std=c++2c %s -verify=expected,since-cxx14 -fexceptions -fcxx-exceptions -pedantic-errors

namespace std {
  __extension__ typedef __SIZE_TYPE__ size_t;

  template<typename T> struct initializer_list {
    const T *p; size_t n;
    initializer_list(const T *p, size_t n);
  };
} // namespace std

namespace cwg1004 { // cwg1004: 5
  template<typename> struct A {};
  template<typename> struct B1 {};
  template<template<typename> class> struct B2 {};
  template<typename X> void f(); // #cwg1004-f-1
  template<template<typename> class X> void f(); // #cwg1004-f-2
  template<template<typename> class X> void g(); // #cwg1004-g-1
  template<typename X> void g(); // #cwg1004-g-2
  struct C : A<int> {
    B1<A> b1a;
    B2<A> b2a;
    void h() {
      f<A>();
      // expected-error@-1 {{call to 'f' is ambiguous}}
      //   expected-note@#cwg1004-f-1 {{candidate function [with X = cwg1004::A<int>]}}
      //   expected-note@#cwg1004-f-2 {{candidate function [with X = cwg1004::A]}}
      g<A>();
      // expected-error@-1 {{call to 'g' is ambiguous}}
      //   expected-note@#cwg1004-g-1 {{candidate function [with X = cwg1004::A]}}
      //   expected-note@#cwg1004-g-2 {{candidate function [with X = cwg1004::A<int>]}}
    }
  };

  // This example (from the standard) is actually ill-formed, because
  // name lookup of "T::template A" names the constructor.
  template<class T, template<class> class U = T::template A> struct Third { };
  // expected-error@-1 {{is a constructor name}}
  //   expected-note@#cwg1004-t {{in instantiation of default argument}}
  Third<A<int> > t; // #cwg1004-t
} // namespace cwg1004

namespace cwg1042 { // cwg1042: 3.5
#if __cplusplus >= 201402L
  // C++14 added an attribute that we can test the semantics of.
  using foo [[deprecated]] = int; // #cwg1042-using
  foo f = 12;
  // since-cxx14-warning@-1 {{'foo' is deprecated}}
  //   since-cxx14-note@#cwg1042-using {{'foo' has been explicitly marked deprecated here}}
#elif __cplusplus >= 201103L
  // C++11 did not have any attributes that could be applied to an alias
  // declaration, so the best we can test is that we accept an empty attribute
  // list in this mode.
  using foo [[]] = int;
#endif
} // namespace cwg1042

namespace cwg1048 { // cwg1048: 3.6
  struct A {};
  const A f();
  A g();
  typedef const A CA;
#if __cplusplus >= 201103L
  // ok: we deduce non-const A in each case.
  A &&a = [] (int n) {
    while (1) switch (n) {
      case 0: return f();
      case 1: return g();
      case 2: return A();
      case 3: return CA();
    }
  } (0);
#endif
} // namespace cwg1048

namespace cwg1054 { // cwg1054: yes
  struct A {} volatile a; // #cwg1054-A
  void f() {
    // An lvalue-to-rvalue conversion is applied here, which copy-initializes a
    // temporary from 'a'. This is ill-formed because A does not have a
    // volatile copy constructor.
#if __cplusplus >= 201103L
    a; // #cwg1054-use
    // expected-error@#cwg1054-use {{no matching constructor for initialization of 'struct A'}}
    //   expected-note@#cwg1054-A {{candidate constructor (the implicit copy constructor) not viable: 1st argument ('volatile struct A') would lose volatile qualifier}}
    //   expected-note@#cwg1054-A {{candidate constructor (the implicit move constructor) not viable: 1st argument ('volatile struct A') would lose volatile qualifier}}
    //   expected-note@#cwg1054-A {{candidate constructor (the implicit default constructor) not viable: requires 0 arguments, but 1 was provided}}
    // expected-warning@#cwg1054-use {{expression result unused; assign into a variable to force a volatile load}}
#endif
  }

  struct B { B(); B(const volatile B &); } volatile b;
  void g() {
#if __cplusplus >= 201103L
    b;
#endif
  }
} // namespace cwg1054

namespace cwg1070 { // cwg1070: 3.5
#if __cplusplus >= 201103L
  struct A {
    A(std::initializer_list<int>);
  };
  struct B {
    int i;
    A a;
  };
  B b = {1};
  struct C {
    std::initializer_list<int> a;
    B b;
    std::initializer_list<double> c;
  };
  C c = {};
#endif
} // namespace cwg1070
// cwg1002: na
// cwg1005: na
// cwg1007: na
// cwg1008: na
// cwg1014: na
// cwg1019: na
// cwg1023: na
// cwg1026: na
// cwg1039: na
// cwg1040: na
// cwg1041: na
// cwg1045: na
// cwg1050: na
// cwg1052: na
// cwg1053: na
// cwg1058: na
// cwg1067: na
// cwg1077: na
// cwg1078: na
// cwg1084: na
// cwg1085: na
// cwg1097: na
// cwg1063: na
// cwg1065: na
// cwg1090: na
// cwg1076: na
// cwg1003: na
// cwg1059: na
// cwg1013: na
// cwg1024: na
// cwg1091: na
// cwg1096: na
// cwg1083: na

namespace cwg1009 { // cwg1009: yes
// The declarator-id of a function template can be an operator-function-id, a
// conversion-function-id, a constructor name, or a literal-operator-id, but a
// destructor cannot be a template.
struct S {
  template<class T> S(T);
  template<class T> operator T*();
  template<class T> S &operator+=(T);
  template<class T> ~S();
  // expected-error@-1 {{destructor cannot be declared as a template}}
};
template<class T> struct X {
  template<class U> operator U();
  template<class U> X &operator+=(U);
};
template<class T> template<class U> X<T>::operator U() { return U(); }
template<class T> template<class U> X<T> &X<T>::operator+=(U) { return *this; }
} // namespace cwg1009

#if __cplusplus >= 201103L
namespace cwg1018 { // cwg1018: yes
// An attribute-declaration and a simple-declaration beginning with an
// attribute-specifier-seq are distinguished by what follows.
[[]];
[[]] int a;
[[]] int f();
void g() {
  [[]];
  [[]] int b;
}
} // namespace cwg1018
#endif

#if __cplusplus >= 201103L
namespace std { struct type_info { constexpr bool operator==(const type_info &o) const { return this == &o; } }; }
namespace cwg1047 { // cwg1047: yes
// typeid(T) with a dependent T is value-dependent: the comparison is not
// evaluated until instantiation.
template<class T> constexpr bool is_int() { return typeid(T) == typeid(int); }
static_assert(is_int<int>(), "");
} // namespace cwg1047
#endif

#if __cplusplus >= 201103L
namespace cwg1056 { // cwg1056: yes
// A member alias template can name the current instantiation.
template<class T> struct B {
  typedef int type;
  template<class U> using X = typename B::type;
  template<class U> using Y = typename B<T>::type;
  X<char> a;
  Y<char> b;
};
B<int> bi;
static_assert(__is_same(B<int>::X<char>, int), "");
static_assert(__is_same(B<int>::Y<char>, int), "");
} // namespace cwg1056
#endif

#if __cplusplus >= 201103L
namespace cwg1057 { // cwg1057: yes
// decltype of a member of the current instantiation is dependent.
template<class T> struct C {
  typedef int I;
  int n;
  decltype(C::I()) x;
  decltype(n) m;
};
C<int> c;
static_assert(__is_same(decltype(c.m), int), "");
} // namespace cwg1057
#endif

#if __cplusplus >= 201103L
namespace cwg1079 { // cwg1079: yes
struct A { int i; };
int f(const A &);
long f(A &&);
int g(A, double);
long g(A, int);
static_assert(sizeof(f({1})) == sizeof(long), "");
static_assert(sizeof(g({1}, 1)) == sizeof(long), "");
} // namespace cwg1079
#endif

#if __cplusplus >= 201103L
namespace cwg1080 { // cwg1080: yes
// A member template is never instantiated to produce a copy or move
// constructor signature.
struct S {
  template<typename T> S(T);
  template<typename T> S(T&&);
  S();
};
S f();
const S g;
void h() {
  S a(f());
  S b(g);
}
static_assert(__is_constructible(S, const S &), "");
} // namespace cwg1080
#endif

namespace cwg1088 { // cwg1088: yes
template<const char *> struct A {};
template<class T> struct D {
  static const char c[1];
  typedef A<c> C;
};
template<class T> const char D<T>::c[1] = {};
#if __cplusplus >= 201103L
static_assert(!__is_same(D<int>::C, D<char>::C), "");
#endif
} // namespace cwg1088
