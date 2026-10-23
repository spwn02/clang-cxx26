// RUN: %clang_cc1 -std=c++17 -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

// [dcl.init.ref]p5.4: otherwise, T1 shall not be reference-related to T2. 'const int *' and
// 'int *' are reference-related but not reference-compatible, so no temporary is created
// (CWG2801).

void f() {
  int *p = nullptr;
  const int *&&r = static_cast<int *&&>(p); // expected-error {{binding reference of type 'const int *' to value of type 'int *' not permitted due to incompatible qualifiers}}
  int *const &ok = p;
  const int *const &ok2 = p; // direct binding after a qualification conversion
  const int *q = p;
  const int *const &ok3 = q;
}

void g(const int *&&); // expected-note {{candidate function not viable: no known conversion from 'int *' to 'const int *&&' for 1st argument}}
void h() {
  int *p = nullptr;
  g(static_cast<int *&&>(p)); // expected-error {{no matching function}}
}
