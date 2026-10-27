// RUN: %clang_cc1 -std=c++98 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++11 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++14 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++17 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++20 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++23 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++2c -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions

namespace cwg2483 { // cwg2483: yes
extern "C" {
struct T {
  static void f();
  void g();
};
}
// The function type of a static member function has C language linkage here
// ([dcl.link]); that of a non-static member function does not.
extern "C" void (*p)() = &T::f;
void (*q)() = &T::f;
// expected-error@-1 {{converting 'extern "C" void (*)()' to 'void (*)()' changes the language linkage of the function type}}
void (T::*r)() = &T::g;
} // namespace cwg2483
