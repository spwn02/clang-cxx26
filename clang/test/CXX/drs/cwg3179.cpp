// RUN: %clang_cc1 -std=c++2c -verify=expected %s

namespace std {
template <class...> using void_t = void;
}

template <class T> struct S {
  void f(std::void_t<T *>); // expected-error {{argument may not have 'void' type}}
};
S<int> x; // expected-note {{in instantiation of template class 'S<int>' requested here}}

template <class T> struct S2 {
  void f(std::void_t<int *>);
};
S2<int> y;

using V = void;
template <class T> struct S3 {
  void f(V);
};
S3<int> z;

template <class T> void g(std::void_t<T *>);
template <class T> auto test(int) -> decltype(g<T>(), int{});
template <class> auto test(...) -> long;
static_assert(__is_same(decltype(test<int>(0)), long));
