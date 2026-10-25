// RUN: %clang_cc1 -std=c++20 -pedantic-errors -triple x86_64-unknown-unknown -emit-module-interface -verify %s -o /dev/null
// RUN: %clang_cc1 -std=c++23 -pedantic-errors -triple x86_64-unknown-unknown -emit-module-interface -verify %s -o /dev/null
// RUN: %clang_cc1 -std=c++2c -pedantic-errors -triple x86_64-unknown-unknown -emit-module-interface -verify %s -o /dev/null

// cwg2443: yes
// An export-declaration shall not declare an explicit instantiation, an
// explicit specialization or a partial specialization with its keyword
// directly; these are allowed inside an export block, except for partial
// specializations of the first form.

export module M;

template<class T> void f(T) {}
export template void f<int>(int);
// expected-error@-1 {{an export declaration cannot declare an explicit instantiation}}

template<class T> void g(T) {}
export template<> void g<char>(char) {}
// expected-error@-1 {{an export declaration cannot declare an explicit specialization}}

template<class T> struct trait {};
export template<class T> struct trait<T *>;
// expected-error@-1 {{an export declaration cannot declare a partial specialization}}

export {
  template<class T> struct A;
  template<class T> struct A<T *>;
}

export template<class T> struct B {};
