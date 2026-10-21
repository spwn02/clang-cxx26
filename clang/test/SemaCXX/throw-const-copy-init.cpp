// RUN: %clang_cc1 -std=c++26 -fcxx-exceptions -fsyntax-only -verify %s

// [except.throw]: copy-initialization of an object of type T from an lvalue of
// type const T in a context unrelated to T shall be well-formed.
struct M {
  M() = default;
  M(M &&) = default;
  M(const M &) = delete; // expected-note {{has been explicitly marked deleted here}}
};
struct P {
  P() = default;
  P(P &&) = default;
private:
  P(const P &) = default; // expected-note {{declared private here}}
};
struct Ok {
  Ok() = default;
  Ok(const Ok &) = default;
};
struct Conv {
  Conv(Conv &) = default; // non-const copy only
  Conv() = default;
};

void f() { throw M{}; }  // expected-error {{call to deleted constructor of 'M'}}
void g() { throw P{}; }  // expected-error {{calling a private constructor of class 'P'}}
void h() { throw Ok{}; }
void i() { throw 1; throw "s"; throw nullptr; }
template <class T> void t() { throw T{}; }
void inst() { t<Ok>(); }
