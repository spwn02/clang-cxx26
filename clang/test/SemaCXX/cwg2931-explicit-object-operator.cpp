// RUN: %clang_cc1 -std=c++23 -fsyntax-only -verify %s

// [over.oper]: an operator function has at least one function parameter or
// implicit object parameter of class or enumeration type (CWG2931).

struct S1 {
  int operator+(this int, int); // expected-error {{overloaded 'operator+' must have at least one parameter of class or enumeration type}}
};
struct S2 {
  int operator+(this S2, int);
};
struct S3 {
  int operator+(this int, S3);
};
