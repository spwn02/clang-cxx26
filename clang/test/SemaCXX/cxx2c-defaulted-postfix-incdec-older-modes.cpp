// P3668R4 only adds defaulted postfix ++/-- in C++26: older modes must keep rejecting them,
// with the diagnostic text of the respective standard.
// RUN: %clang_cc1 -std=c++11 -fsyntax-only -verify=pre20 %s
// RUN: %clang_cc1 -std=c++14 -fsyntax-only -verify=pre20 %s
// RUN: %clang_cc1 -std=c++17 -fsyntax-only -verify=pre20 %s
// RUN: %clang_cc1 -std=c++20 -fsyntax-only -verify=cxx20 %s
// RUN: %clang_cc1 -std=c++23 -fsyntax-only -verify=cxx20 %s

struct P {
  P operator++(int) = default;
  // pre20-error@-1 {{only special member functions may be defaulted}}
  // cxx20-error@-2 {{only special member functions and comparison operators may be defaulted}}
  P operator--(int) = default;
  // pre20-error@-1 {{only special member functions may be defaulted}}
  // cxx20-error@-2 {{only special member functions and comparison operators may be defaulted}}
  P &operator++() = default;
  // pre20-error@-1 {{only special member functions may be defaulted}}
  // cxx20-error@-2 {{only special member functions and comparison operators may be defaulted}}
};
