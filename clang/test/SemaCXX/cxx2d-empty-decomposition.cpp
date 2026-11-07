// RUN: %clang_cc1 -std=c++2d -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++2c -fsyntax-only -verify=pre %s

struct Empty {};
struct One { int value; };

void empty_structured_binding() {
  auto [] = Empty{}; // pre-warning {{ISO C++17 does not allow a structured binding group to be empty}}
  auto [] = One{};   // expected-error {{type 'One' binds to 1 element, but no names were provided}} pre-error {{type 'One' binds to 1 element, but no names were provided}} pre-warning {{ISO C++17 does not allow a structured binding group to be empty}}
}
