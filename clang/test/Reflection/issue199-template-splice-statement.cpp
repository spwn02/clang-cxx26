// RUN: %clang_cc1 -std=c++26 -freflection -verify %s
// expected-no-diagnostics
template<class T> int h(T v) { return v; }
constexpr auto r = ^^h;
template<int> struct C { using type = int; static void member() {} };
void f() {
  template [:r:]<int>(1);
  template [:r:](1);
  template [:^^C:]<0>::member();
}
template<auto R> void dep() {
  template [:R:]<int>(1);
  template [:R:](1);
}
void g() { dep<r>(); }
