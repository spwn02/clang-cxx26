// RUN: %clang_analyze_cc1 -std=c++26 -fcontracts -analyzer-checker=core -verify %s

// A contract assertion does not end the analyzed path (#316).
void afterAssertion(int x) {
  contract_assert(x > 0);
  int *p = nullptr;
  *p = 1; // expected-warning {{Dereference of null pointer}}
}

void noAssertion() {
  int *p = nullptr;
  *p = 1; // expected-warning {{Dereference of null pointer}}
}
