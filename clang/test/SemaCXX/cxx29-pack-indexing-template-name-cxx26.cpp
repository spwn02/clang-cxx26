// RUN: %clang_cc1 -std=c++2c -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++23 -fsyntax-only -verify %s

// P3670R4 is C++29 only: neither an extension nor a warning in earlier modes.

template <class T> struct A {};
template <template <typename> typename... TT>
struct S {
  template <typename T> using First = TT...[0]<T>; // expected-error {{pack indexing for template names is only available in C++29}} \
                                                   // expected-error {{expected ';' after alias declaration}}
};
