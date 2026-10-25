// RUN: %clang_cc1 -fsyntax-only -verify -Wc++11-compat %s
// RUN: %clang_cc1 -fsyntax-only -verify -std=c++98 -Wc++11-compat %s
// RUN: %clang_cc1 -fsyntax-only -verify -std=c++11 %s

namespace N {
  template<class T> class Y {
    void mf() { } 
  };
}

template class Z<int>; // expected-error{{explicit instantiation of undeclared template class 'Z'}}

struct Q;
template class Q<int>; // expected-error{{explicit instantiation of non-template class 'Q'}}

using N::Y; 
template class Y<int>;

template class N::Y<char*>; 
template void N::Y<double>::mf();
