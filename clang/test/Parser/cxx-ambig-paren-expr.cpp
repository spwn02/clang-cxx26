// RUN: %clang_cc1 -fsyntax-only -pedantic -verify %s

void f() {
  typedef int T;
  int x, *px;
  
  // A function type is not a nofun-type-id, so these are expressions
  // (CWG2228).
  (T())x;    // expected-error {{expected ';' after expression}} expected-warning 2 {{expression result unused}}
  (T())+x;   // expected-warning {{expression result unused}}
  (T())*px;  // expected-error {{invalid operands to binary expression}}
  
  // Expression.
  x = (T());
  x = (T())/x;

  typedef int *PT;
  // Make sure stuff inside the parens are parsed only once (only one warning).
  x = (PT()[(int){1}]); // expected-warning {{compound literals}}

  // Special case: empty parens is a call, not an expression
  struct S{int operator()();};
  (S())();

  // Special case: "++" is postfix here, not prefix
  (S())++; // expected-error {{cannot increment value of type 'S'}}

  struct X { int &operator++(int); X operator[](int); int &operator++(); };
  int &postfix_incr = (X()[3])++;
  (X())++ ++; // ok, not a C-style cast
  (X())++ ++X(); // expected-error {{expected ';' after expression}}
  int q = (int)++(x);
}

// Make sure we do tentative parsing correctly in conditions.
typedef int type;
struct rec { rec(int); };

namespace ns {
  typedef int type;
  struct rec { rec(int); };
}

struct cls {
  typedef int type;
  struct rec { rec(int); };
};

struct result {
  template <class T> result(T);
  bool check();
};

void test(int i) {
  if (result((cls::type) i).check())
    return;

  if (result((ns::type) i).check())
    return;

  if (result((::type) i).check())
    return;

  if (result((cls::rec) i).check())
    return;

  if (result((ns::rec) i).check())
    return;

  if (result((::rec) i).check())
    return;
}

