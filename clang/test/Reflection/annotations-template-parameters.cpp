// RUN: %clang_cc1 %s -std=c++26 -freflection -fannotation-attributes -verify
// RUN: %clang %s -std=c++26 -freflection-latest -fsyntax-only -Xclang -verify

// Template parameters are not annotation targets, including in dependent
// declarations that are never instantiated.
template<int I [[=7]]> struct A {}; // expected-error {{annotation cannot be applied to a template parameter}}
template<class T [[=1]]> struct B {}; // expected-error {{annotation cannot be applied to a template parameter}}
template<template<class> class TT [[=2]]> struct C {}; // expected-error {{annotation cannot be applied to a template parameter}}
template<int... Is [[=3]]> struct D {}; // expected-error {{annotation cannot be applied to a template parameter}}
template<typename T [[=1]] = int> struct E {}; // expected-error {{annotation cannot be applied to a template parameter}}
template<class... Ts [[=1]]> struct F {}; // expected-error {{annotation cannot be applied to a template parameter}}
template<template<class> class... Ts [[=1]]> struct G {}; // expected-error {{annotation cannot be applied to a template parameter}}
template<class T, T V [[=sizeof(T)]]> void dependent(); // expected-error {{annotation cannot be applied to a template parameter}}
template<[[=1]] int I> struct Leading {}; // expected-error {{an attribute list cannot appear here}}
template<int I [[=1, =2]] = 0> struct Multiple {}; // expected-error 2 {{annotation cannot be applied to a template parameter}}
template<class T, int I [[=sizeof(T)]] = 0>
auto overload(T) -> decltype(T::missing); // expected-error@-1 {{annotation cannot be applied to a template parameter}}
void overload(...);
void use_overload() { overload(0); }

// Annotations on the template's declaration and on function parameters remain
// valid, including dependent annotations and explicit specializations.
template<class T> struct [[=1]] S {
  template<class U> struct [[=2]] Member {};
  template<class U> [[=3]] void member([[=4]] U);
};
template<> struct [[=5]] S<int> {};
template<class T> [[=2]] T variable;
template<> [[=3]] int variable<int>;
template<class T> using Alias [[=4]] = T;
template<class T> [[=sizeof(T)]] void function([[=3]] T x) {}
template<> [[=6]] void function<int>([[=7]] int x) {}
void ordinary([[=8]] int x) {}
auto lambda = []([[=4]] int x) {};
auto generic_lambda = []([[=5]] auto x) {};
void instantiate() { function('x'); generic_lambda(0); }

// Preserve existing treatment of ordinary attributes on non-type parameters.
template<int I [[maybe_unused]]> struct Ordinary {};
