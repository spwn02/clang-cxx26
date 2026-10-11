// RUN: %clang_cc1 -std=c++20 -verify %s
// expected-warning@44 {{empty struct has size 0 in C, size 1 in C++}}

namespace GH46386 {
  extern "C" {
  template<class T> void f(T) {}
  void takes_int(int);
  void use_template() { f(1); }
  }
}

namespace extern_c_templates {
template<typename T> struct A {
  static int a;
  struct b;
  void c();
  enum class d;
  template<typename U> static int e;
  template<typename U> struct f;
  template<typename U> void g();
};
template<typename T> int B;
template<typename T> void C();

extern "C" {
  template<typename T> struct D;
  template<typename T> int B2;
  template<typename T> void C2();
  template<typename T> int A<T>::a;
  template<typename T> struct A<T>::b {};
  template<typename T> void A<T>::c() {}
  template<typename T> enum class A<T>::d {};
  template<typename T> template<typename U> int A<T>::e;
  template<typename T> template<typename U> struct A<T>::f {};
  template<typename T> template<typename U> void A<T>::g() {}
  template<> struct D<int> { int member; };
  template<> int B2<int>;
  template<> void C2<int>() {}
}

template<class T> struct Partial;
extern "C" {
  template<class T> struct Partial<T *> {};
  template<> struct Partial<int> {};
}

extern "C" template<class T> void single(T) {}
extern "C" { template<class T> void block(T) {} }

extern "C" {
  template<class T> int same(T) { return 0; }
}
template int same<int>(int);
template int same<char>(char);

}

template<class T> extern "C" void invalid_single(T); // expected-error {{a linkage-specification cannot appear within a template declaration}}
template<class T> extern "C" { void invalid_block(T); } // expected-error {{a linkage-specification cannot appear within a template declaration}}
