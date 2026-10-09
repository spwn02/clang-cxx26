// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

// P3865R3: class template argument deduction for type template template parameters. When the placeholder for a deduced
// class type designates a type template template parameter P, the alias template whose template parameter list is that
// of P and whose defining-type-id is the template argument for P applied to the template parameters of P is used.

template <class A, class B> struct is_same { static constexpr bool value = false; };
template <class A> struct is_same<A, A> { static constexpr bool value = true; };

template <typename... Ts> struct Y {
  Y();
  Y(Ts...);
};

namespace paper_example {
template <template <typename T = char> class X> void f() {
  X x;        // OK, deduces Y<char> // #paper_x
  X x0{};     // OK, deduces Y<char>
  X x1{1};    // OK, deduces Y<int>
  X x2{1, 2}; // expected-error {{no viable constructor or deduction guide for deduction of template arguments of 'X'}}
  // expected-note@#paper_x 3 {{candidate function template not viable: requires}}
  // expected-note@#paper_x 3 {{implicit deduction guide declared as 'template <typename T = char> requires __is_deducible(X, Y<T>)}}
  static_assert(is_same<decltype(x), Y<char>>::value, "");
  static_assert(is_same<decltype(x0), Y<char>>::value, "");
  static_assert(is_same<decltype(x1), Y<int>>::value, "");
}
template void f<Y>(); // expected-note {{in instantiation of function template specialization}}
} // namespace paper_example

namespace uninstantiated {
// The deduction is deferred until the enclosing template is instantiated.
template <template <typename T = char> class X> void f() {
  X x;
  X x0{};
  X x1{1};
}
} // namespace uninstantiated

namespace pack {
template <template <typename... Ts> class X> void f() {
  X x{1, 2};
  static_assert(is_same<decltype(x), Y<int, int>>::value, "");
}
template void f<Y>();
} // namespace pack

namespace alias_argument {
template <typename T> using AY = Y<T>;
template <template <typename> class X> void f() {
  X x{1.5};
  static_assert(is_same<decltype(x), Y<double>>::value, "");
}
template void f<AY>();
} // namespace alias_argument

namespace user_guide {
template <typename T> struct Z {
  Z(T);
};
template <template <typename> class X> void f() {
  X a{1};
  static_assert(is_same<decltype(a), Z<int>>::value, "");
}
template void f<Z>();
} // namespace user_guide

namespace non_type {
template <typename T, int N> struct W {
  W(T);
};
template <template <typename T, int N = 0> class X> void f() {
  X x{1.0f};
  static_assert(is_same<decltype(x), W<float, 0>>::value, "");
}
template void f<W>();
} // namespace non_type

namespace vector_like {
template <typename T, typename Alloc = int> struct V {
  V(T);
};
template <template <typename> class X> void one() {
  // The argument has an extra defaulted parameter, which template template argument matching permits.
  X x{1};
  static_assert(is_same<decltype(x), V<int>>::value, "");
}
template <template <typename...> class X> void many() {
  X x{1};
  static_assert(is_same<decltype(x), V<int>>::value, "");
  auto y = X(2.5);
  static_assert(is_same<decltype(y), V<double>>::value, "");
  auto *z = new X{'c'};
  static_assert(is_same<decltype(z), V<char> *>::value, "");
  delete z;
}
template void one<V>();
template void many<V>();
} // namespace vector_like

namespace deduced_argument {
template <template <typename> class X, typename T> auto h(const X<T> &) {
  X y{T()};
  return y;
}
template <typename T> struct S {
  S(T);
};
static_assert(is_same<decltype(h(S<int>(1))), S<int>>::value, "");
} // namespace deduced_argument

namespace in_lambda {
template <typename T> struct Z2 {
  Z2(T);
};
template <template <typename> class X> void f() {
  auto l = [](auto v) {
    X x{v};
    return x;
  };
  static_assert(is_same<decltype(l(1)), Z2<int>>::value, "");
}
template void f<Z2>();
} // namespace in_lambda

namespace not_deducible {
template <typename T> struct NoGuide {
  NoGuide(int);
};
template <template <typename> class X> void f() {
  X x{1}; // expected-error {{no viable constructor or deduction guide}}
  // expected-note@-1 {{candidate template ignored: couldn't infer template argument}}
  // expected-note@-2 {{candidate template ignored: could not match}}
  // expected-note@-3 2 {{implicit deduction guide declared as 'template <typename> requires __is_deducible(X,}}
}
template void f<NoGuide>(); // expected-note {{in instantiation}}
} // namespace not_deducible

namespace variadic_parameter_nonvariadic_alias {
// A pack of the template template parameter cannot be applied to the single parameter of an alias template; the
// deduction then goes through the template argument itself (std::ranges::to<Alias> relies on this).
template <typename T> struct W {
  W(T);
};
template <typename T> using WA = W<T>;
template <template <typename...> class C> auto f1() { return C(1); }
template <template <typename...> class C, typename U> auto f2(U u) { return C(u); }
static_assert(is_same<decltype(f1<W>()), W<int>>::value, "");
static_assert(is_same<decltype(f1<WA>()), W<int>>::value, "");
static_assert(is_same<decltype(f2<WA>(2.0)), W<double>>::value, "");
} // namespace variadic_parameter_nonvariadic_alias
