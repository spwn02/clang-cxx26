// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s

// [dcl.contract.func]: "The function contract assertions of a function are
// considered to be needed ... when ... the function is odr-used or ... defined."
template<class T> void addressed(T t) pre(t.no_such_member()); // expected-error {{member reference base type 'const int' is not a structure or union}}
auto p = &addressed<int>; // expected-note {{in instantiation}}
template<class T> void called(T t) pre(t.no_such_member()); // expected-error {{member reference base type 'const int' is not a structure or union}}
void use() { called(1); } // expected-note {{in instantiation}}

template<class T> void unused(T t) pre(t.no_such_member());
using Unevaluated = decltype(&unused<int>);

struct Valid { bool no_such_member() const; };
auto valid = &addressed<Valid>;

// [dcl.contract.func]: "Overload resolution does not consider
// function-contract-specifiers." Draft example, verbatim:
namespace draft_example {
template <typename T>  void f(T t) pre( t == "" ); // expected-note {{candidate function}}
template <typename T>  void f(T&& t); // expected-note {{candidate function}}
void g()
{
  f(5);     // expected-error {{call to 'f' is ambiguous}}
}
}

// [dcl.contract.func]: assertions are "needed" when "the function is odr-used".
template<class T> struct Member {
  void f(T t) pre(t.no_such_member()); // expected-error {{member reference base type 'const int' is not a structure or union}}
};
auto member = &Member<int>::f; // expected-note {{in instantiation}}

// Taking an address needs assertions, but does not require an unavailable body.
template<class T> void good(const T t) pre(t > 0);
auto good_address = &good<int>;
