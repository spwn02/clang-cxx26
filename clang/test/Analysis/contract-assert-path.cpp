// RUN: %clang_analyze_cc1 -std=c++26 -fcontracts -analyzer-checker=core -verify=expected,enforce %s
// RUN: %clang_analyze_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=quick_enforce -analyzer-checker=core -verify=expected,enforce %s
// RUN: %clang_analyze_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=observe -analyzer-checker=core -verify=expected,observe %s
// RUN: %clang_analyze_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=ignore -analyzer-checker=core -verify=expected,ignore %s

// A contract assertion does not end the analyzed path (#316). With the enforce and quick_enforce semantics the path
// continues only where the predicate holds; with observe it continues unconstrained; with ignore the predicate is not
// evaluated at all ([basic.contract.eval]), so the analyzer does not model it (#317).
void afterAssertion(int x) {
  contract_assert(x > 0);
  int *p = nullptr;
  *p = 1; // expected-warning {{Dereference of null pointer}}
}

void noAssertion() {
  int *p = nullptr;
  *p = 1; // expected-warning {{Dereference of null pointer}}
}

void checked(int *p) {
  contract_assert(p != nullptr);
  *p = 1; // observe-warning {{Dereference of null pointer}}
}

void checkedAfterBranch(int *p) {
  if (!p) {
  }
  contract_assert(p != nullptr);
  *p = 1; // observe-warning {{Dereference of null pointer}} ignore-warning {{Dereference of null pointer}}
}

void contradicted(int *p) {
  contract_assert(p == nullptr);
  *p = 1; // enforce-warning {{Dereference of null pointer}} observe-warning {{Dereference of null pointer}}
}

void knownNull() {
  int *p = nullptr;
  contract_assert(p != nullptr);
  *p = 1; // observe-warning {{Dereference of null pointer}} ignore-warning {{Dereference of null pointer}}
}

int global;
bool touch() {
  global = 1;
  return true;
}

void predicateEffects() {
  global = 0;
  contract_assert(touch());
  int z = 10 / global; // ignore-warning {{Division by zero}}
  (void)z;
}
