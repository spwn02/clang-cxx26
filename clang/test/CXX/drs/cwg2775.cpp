// RUN: %clang_cc1 -std=c++2c -pedantic-errors %s -verify -fexceptions -fcxx-exceptions
// expected-note@*:* 0+ {{}}

namespace cwg2775 { // cwg2775: 22
struct T {
  T();
  T(T &&);
  T(const T &) = delete;
};

void f() {
  T t;
  throw t;
  // expected-error@-1 {{call to deleted constructor of 'T'}}
}

struct U {
  U();
  U(const U &);
};

void g() {
  U u;
  throw u;
}
} // namespace cwg2775
