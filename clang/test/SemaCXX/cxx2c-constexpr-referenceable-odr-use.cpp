// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify -verify-ignore-unexpected=note %s
// P2686R5 / [basic.def.odr]: odr-use of variables whose value holds the address of automatic objects.
// [basic.def.odr]: a variable whose value holds the address of an automatic
// object is not constexpr-representable inside a nested function scope, so
// naming it there (with the lvalue-to-rvalue conversion) odr-uses it.
namespace lambda_odr_use {
struct Holder { int &r; };
int pointer_without_capture() {
  int x = 1;
  constexpr int *p = &x;
  return [] { return *p; }(); // expected-error {{variable 'p' cannot be implicitly captured in a lambda with no capture-default specified}}
 
}
int reference_member_without_capture() {
  int x, y;
  constexpr Holder b1 = {x}, b2 = {y};
  return [] { return b1.r; }(); // expected-error {{variable 'b1' cannot be implicitly captured in a lambda with no capture-default specified}}
 
}
int explicit_and_default_capture() {
  int x = 1;
  constexpr int *p = &x;
  auto by_copy = [p] { return *p; };
  auto by_default = [&] { return *p; };
  return by_copy() + by_default();
}
constexpr int recursive(int n) {
  int a = n * 10;
  constexpr int *p = &a; // each call frame refers to its own 'a'
  return n ? recursive(n - 1) + *p : *p;
}
static_assert(recursive(3) == 60);
constexpr int *escapes() {
  int a = 1;
  constexpr int *p = &a;
  return p; // expected-warning {{address of stack memory associated with local variable 'a' returned}}
 
}
constexpr int *dangling = escapes(); // expected-error {{must be initialized by a constant expression}}
}

namespace templates {
template <class T> int generic_with_capture_default() {
  int x = 1;
  constexpr int *p = &x;
  return [&](auto) { return *p; }(0);
}
template <class T> int explicit_capture() {
  int x = 2;
  constexpr int *p = &x;
  auto l = [p](auto) { return *p; };
  return l(0);
}
template <class T> int generic_without_capture() {
  int x = 1;
  constexpr int *p = &x;
  return [](auto) { return *p; }(0); // expected-error 1+ {{variable 'p' cannot be implicitly captured in a lambda with no capture-default specified}}
}
template <class T> int local_class() {
  int x = 1;
  constexpr int *p = &x;
  struct L {
    int f() { return *p; } // expected-error {{reference to local variable 'p' declared in enclosing function}}
  };
  return L{}.f();
}
int use() {
  return generic_with_capture_default<int>() + explicit_capture<int>() +
         generic_without_capture<int>() + local_class<int>();
}
}
