// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s
// expected-no-diagnostics

// [temp.deduct.type]: only the constant-expression of a splice-specifier is a
// non-deduced context. A splice-specialization-specifier whose template is
// known is an ordinary template-id: the arguments written after it deduce.

namespace std::meta { using info = decltype(^^int); }

template <class...> struct X {};
template <class, class> struct Y {};
template <class T> struct is_x { static constexpr bool v = false; };
template <class... Ts> struct is_x<typename [:^^X:]<Ts...>> {
  static constexpr bool v = true;
};
static_assert(is_x<X<int, char>>::v);
static_assert(!is_x<int>::v);

template <class T> struct first { using type = void; };
template <class A, class B> struct first<typename [:^^Y:]<A, B>> {
  using type = A;
};
static_assert(__is_same(first<Y<int, long>>::type, int));

// A function template, deduced through the spliced template-id.
template <class... Ts> constexpr int count(typename [:^^X:]<Ts...>) {
  return sizeof...(Ts);
}
static_assert(count(X<int, int, int>{}) == 3);
