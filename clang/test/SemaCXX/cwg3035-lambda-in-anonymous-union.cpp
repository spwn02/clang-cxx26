// RUN: %clang_cc1 -std=c++20 -fsyntax-only -verify %s

// [class.union.anon]: nested types, including closure types, cannot be declared
// within an anonymous union (CWG3035).

struct S {
  union {
    int i = []{ return 1; }(); // expected-error {{types cannot be declared in an anonymous union}}
    char c;
  };
  union {
    int j;
    char d;
  };
  int k = []{ return 3; }();
};
