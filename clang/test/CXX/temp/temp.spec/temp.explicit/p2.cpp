// RUN: %clang_cc1 -fsyntax-only -verify -pedantic -Wc++11-compat %s
// RUN: %clang_cc1 -fsyntax-only -verify -pedantic -std=c++98 -Wc++11-compat %s
// RUN: %clang_cc1 -fsyntax-only -verify -pedantic -std=c++11 %s

// Example from the standard
template<class T> class Array { void mf() { } }; 

template class Array<char>; 
template void Array<int>::mf();
template<class T> void sort(Array<T>& v) { /* ... */ }
template void sort(Array<char>&);
namespace N { 
  template<class T> void f(T&) { }
} 
template void N::f<int>(int&);


template<typename T>
struct X0 {
  struct Inner {};
  void f() { }
  static T value;
};

template<typename T>
T X0<T>::value = 17;

typedef X0<int> XInt;

template struct XInt::Inner; // expected-warning{{template-id}}
template void XInt::f(); // expected-warning{{template-id}}
template int XInt::value; // expected-warning{{template-id}}

namespace N {
  template<typename T>
  struct X1 {
  };
  
  template<typename T>
  void f1(T) {}
}
using namespace N;

template struct X1<int>;

// The declarator-id of a function template specialization is not looked up
// through a using-directive.
template void f1(int);
// expected-error@-1 {{explicit instantiation of 'f1' does not refer to a function template, variable template, member function, member class, or static data member}}
