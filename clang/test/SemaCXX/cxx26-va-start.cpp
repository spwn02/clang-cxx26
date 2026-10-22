// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

// [cstdarg.syn]: va_start(V, ...) takes one argument in C++26; the second and
// subsequent arguments are discarded.
#include <stdarg.h>

void one(int, ...) {
  va_list ap;
  va_start(ap);
  va_end(ap);
}

void two(int n, ...) {
  va_list ap;
  va_start(ap, n);
  va_end(ap);
}

void discarded(int, ...) {
  va_list ap;
  va_start(ap, unrelated tokens here);
  va_end(ap);
}

// expected-no-diagnostics
