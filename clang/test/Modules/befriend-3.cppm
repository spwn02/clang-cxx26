// RUN: %clang_cc1 -std=c++20 %s -fsyntax-only -verify
// expected-no-diagnostics
export module m;

// CWG386: a qualified friend declaration names the function found through a
// using-declaration; the function is a friend of the class.
namespace test {
namespace ns1 {
    namespace ns2 {
    template<class T> void f(T t);
    }
    using ns2::f;
}
struct A { void f(); };
struct B : public A { using A::f; };
template<typename T> struct C : A { using A::f; };
struct X {
    template<class T> friend void ns1::f(T t);
    friend void B::f();
    friend void C<int>::f();
};
}
