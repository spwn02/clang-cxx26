// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -ast-print -verify %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -ast-dump -verify %s

// [dcl.contract.func]: "pre attribute-specifier-seq(opt) ( conditional-expression )".
int malformed() pre(r: true); // expected-error {{result name 'r' not allowed outside of post condition specifier}}

// [dcl.contract.func]: "A deleted function, or a function defaulted on its
// first declaration shall not have a function-contract-specifier-seq."
void deleted() pre(true) = delete; // expected-error {{a deleted function cannot have a function-contract-specifier-seq}}
struct FirstDefault {
  FirstDefault() pre(true) = default; // expected-error {{function defaulted on its first declaration cannot have a function-contract-specifier-seq}}
};
struct LaterDefault {
  LaterDefault() pre(true);
};
LaterDefault::LaterDefault() = default;

// [dcl.contract.func]: "that parameter and the corresponding parameter on all
// declarations of f shall have const type." "This requirement applies even to
// declarations that do not specify the postcondition-specifier."
int redeclared(const int i) post(r: r == i); // expected-error {{parameter 'i' referenced in contract postcondition must be declared const}}
int redeclared(int i); // expected-note {{parameter of type 'int' is declared here}}
int renamed(const int i) post(r: r == i);
int renamed(const int j);
int renamed(const int k) { return k; }

// [dcl.contract.func]: "Parameters with array or function type will decay to
// non-const types even if a const qualifier is present."
int pointer_ok(int (*const p)()) post(r: r == p());
int pointer_bad(int (*p)()) post(r: r == p()); // expected-error {{must be declared const}} expected-note {{parameter of type}}
int function_bad(int p()) post(r: r == p()); // expected-error {{cannot have a function type}} expected-note {{parameter of type}}
// Draft example, verbatim:
int f(const int i[10])
  post(r : r == i[0]);  // expected-error {{cannot have an array type}}
// expected-note@-2 {{parameter of type}}

// [dcl.contract.res]: "attributed-identifier: identifier attribute-specifier-seq(opt)";
// "result-name-introducer: attributed-identifier :".
int attributed() post(r [[maybe_unused]]: true);
int assertion_attributes() pre [[vendor::x]] (true) // expected-warning {{unknown attribute 'vendor::x' ignored}}
  post [[vendor::x]] (r [[maybe_unused]]: r > 0); // expected-warning {{unknown attribute 'vendor::x' ignored}}
struct DelayedAttributes {
  int member() pre [[vendor::x]] (true) // expected-warning {{unknown attribute 'vendor::x' ignored}}
    post [[vendor::x]] (r [[maybe_unused]]: r > 0); // expected-warning {{unknown attribute 'vendor::x' ignored}}
};

// [basic.scope.contract]: a result-name-introducer potentially conflicting with
// a declaration targeting "the function parameter scope of F" or, for a
// lambda-declarator, "the nearest enclosing lambda scope" is ill-formed.
void capture_conflict() {
  auto l = [r = 1]() post(r: true) { return 1; }; // expected-error {{result name 'r' conflicts with lambda capture}} expected-note {{previous declaration}}
}
void enclosing_parameter(int r) {
  auto l = []() post(r: true) { return 1; };
}
int own_parameter(int r) post(r: true); // expected-error {{result name 'r' shadows parameter}} expected-note {{previous declaration}}
void lambda_parameter() {
  auto l = [](int r) post(r: true) { return 1; }; // expected-error {{result name 'r' shadows parameter}} expected-note {{previous declaration}}
}
