// RUN: %clang_cc1 -std=c++20 -fsyntax-only -verify %s

template<typename T> concept C = true;

struct A {
    template<C T> void f();
};

auto L = []<C T>{};

template<typename X>
class Friends {
    template<C T> friend void A::f();
    template<C T> friend void decltype(L)::operator()();
    // expected-error@-1 {{a 'decltype' specifier cannot be used in a declarative nested name specifier}}
};
