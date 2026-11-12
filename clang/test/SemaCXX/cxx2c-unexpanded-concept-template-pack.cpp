// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++29 -fsyntax-only -verify %s

template <class T> concept Any = true;

template <template <class> concept... CC> struct S {
  template <CC T> void f(T); // expected-error {{type constraint contains unexpanded parameter pack 'CC'}}
  void g(CC auto);           // expected-error {{type constraint contains unexpanded parameter pack 'CC'}}

  template <class T>
  static constexpr bool r = requires(T x) { { x } -> CC; };
  // expected-error@-1 {{type constraint contains unexpanded parameter pack 'CC'}}
};

template <class, template <class> concept> struct Box {};
template <template <class> concept... CC>
struct InTypeRequirement {
  template <class T>
  static constexpr bool r = requires { typename Box<T, CC>; };
  // expected-error@-1 {{unexpanded parameter pack 'CC'}}
};

template <template <class> concept... CC>
struct Expanded {
  template <CC... Ts> struct types {};
};
