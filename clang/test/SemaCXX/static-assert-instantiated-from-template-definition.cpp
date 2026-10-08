// RUN: %clang_cc1 -std=c++20 -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

// A function template can be instantiated while the definition of another template is parsed (a non-dependent
// constant evaluation in its body or in a member initializer). The scope of the parser is then the scope of the
// template definition, which must not make the failing static_assert of the instantiation "have no effect".

template <typename> constexpr int f() {
  static_assert(false, "boom"); // expected-error {{static assertion failed: boom}}
  return 0;
}

template <bool B> constexpr int g() {
  return f<int>(); // expected-note {{in instantiation of function template specialization 'f<int>' requested here}}
}

template <typename> constexpr int h() {
  static_assert(false, "bang"); // expected-error {{static assertion failed: bang}}
  return 0;
}

template <bool B> struct S {
  static constexpr int v = h<int>(); // expected-note {{in instantiation of function template specialization 'h<int>' requested here}}
};

// A dependent static_assert in a template definition still has no effect until the instantiation.
template <typename T> constexpr int ok() {
  static_assert(sizeof(T) > 0);
  return 1;
}
template <bool B> constexpr int use() { return ok<int>(); }
static_assert(use<true>() == 1);
