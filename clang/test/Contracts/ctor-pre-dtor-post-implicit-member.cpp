// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

// [expr.prim.id.general]: the implicit transformation of an id-expression naming a
// non-static member into a member access through (*this) is ill-formed in the predicate
// of a precondition assertion of a constructor or of a postcondition assertion of a
// destructor. An explicit this-> is fine, and so are other functions and unevaluated
// operands.

struct C {
  bool b;
  C() pre(b) {}                // expected-error {{use of non-static member 'b' without an explicit object expression is not allowed in the precondition of a constructor}}
  C(int) pre(this->b || true) {}
  C(long) pre(sizeof(b) > 0) {}
  C(char) post(b) {}           // a postcondition of a constructor is fine
  ~C() post(b) {}              // expected-error {{use of non-static member 'b' without an explicit object expression is not allowed in the postcondition of a destructor}}
  void f() pre(b) post(b) {}
};

struct D {
  int x;
  D() pre(x > 0);              // expected-error {{use of non-static member 'x' without an explicit object expression is not allowed in the precondition of a constructor}}
  ~D() post(x > 0);            // expected-error {{use of non-static member 'x' without an explicit object expression is not allowed in the postcondition of a destructor}}
};
D::D() {}
D::~D() {}
