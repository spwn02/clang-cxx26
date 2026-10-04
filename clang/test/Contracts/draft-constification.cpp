// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s
// [expr.prim.id.unqual], verbatim Y example: "then the type of the
// expression is const T" for outside object variables, reference variables
// or template parameters, and structured bindings. Each error/OK below is
// marked exactly as in that example.
// [dcl.contract.res]: "An id-expression that names a result binding is a
// const lvalue"; the example also tests copying that binding into a closure.
int n = 0;
struct X { bool m(); }; // expected-note {{'m' declared here}}

struct Y {
  int z = 0;

  void f(int i, int* p, int& r, X x, X* px)
    pre (++n)       // expected-error {{}}
    pre (++i)       // expected-error {{}}
    pre (++(*p))    // OK
    pre (++r)       // expected-error {{}}
    pre (x.m())     // expected-error {{}} expected-note {{'this' is 'const' within contract introduced here}}
    pre (px->m())   // OK
    pre ([=,&i,*this] mutable {
      ++n;          // expected-error {{}}
      ++i;          // expected-error {{}}
      ++p;          // OK, refers to member of closure type
      ++r;          // OK, refers to non-reference member of closure type
      ++this->z;    // OK, captured *this
      // [expr.prim.id.unqual] example: "OK, captured *this" (both accesses).
      ++z;          // OK, captured *this
      int j = 17;
      [&]{
        int k = 34;
        // [expr.prim.lambda.capture]: "An id-expression within the
        // compound-statement of a lambda-expression that is an odr-use of a
        // reference captured by reference refers to the entity to which the
        // captured reference is bound and not to the captured reference".
        ++i;    // expected-error {{}}
        ++j;    // OK
        ++k;    // OK
      }();
      return true;
    }());

  template <int N, int& R, int* P>
  void g()
    pre(++N)        // expected-error {{}}
    pre(++R)        // expected-error {{}}
    pre(++(*P));    // OK

  int h()
    post(r : ++r)   // expected-error {{}}
    post(r: [=] mutable {
       ++r;         // OK, refers to member of closure type
       return true;
     }());

  int& k()
    post(r : ++r);  // expected-error {{}}
};
