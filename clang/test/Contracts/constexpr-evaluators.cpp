// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify=expected,enforce,checked %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify=expected,enforce,checked -fexperimental-new-constant-interpreter %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=quick_enforce -fsyntax-only -verify=expected,enforce,checked -fexperimental-new-constant-interpreter %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=observe -fsyntax-only -verify=expected,observe,checked %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=observe -fsyntax-only -verify=expected,observe,checked -fexperimental-new-constant-interpreter %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=ignore -fsyntax-only -verify=ignore %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=ignore -fsyntax-only -verify=ignore -fexperimental-new-constant-interpreter %s

// Contract assertions in constant evaluation are checked by both constant evaluators (#315): preconditions,
// postconditions (with the result name), and contract_assert. A violation with the observe semantic is a warning that
// does not stop the evaluation; with the ignore semantic nothing is evaluated.

// ignore-no-diagnostics

constexpr int pre1(int x) pre(x > 0) { return x; }
static_assert(pre1(1) == 1);

constexpr int assert1(int x) {
  contract_assert(x > 0);
  return x;
}
static_assert(assert1(1) == 1);

constexpr int post1(const int x) post(r: r == 2) { return x; }
static_assert(post1(2) == 2);

// A class object: the result name is the object that is returned.
struct S {
  const S *self;
  constexpr S() : self(this) {}
  constexpr S(const S &) : self(this) {}
};
constexpr S mk() post(r: r.self == &r) { return S(); }
constexpr bool useMk() {
  S s = mk();
  return s.self == &s;
}
static_assert(useMk());

// A reference: the result name denotes the referenced object.
constexpr int global = 4;
constexpr const int &ref(const int &v) post(r: &r == &v) { return v; }
static_assert(&ref(global) == &global);

// void functions and every return statement.
constexpr void voidPost(int &i) post(i == 3) { i = 3; }
constexpr int callVoid() {
  int i = 0;
  voidPost(i);
  return i;
}
static_assert(callVoid() == 3);

constexpr int twoReturns(const int x) post(r: r > 0) {
  if (x > 10)
    return 11;
  return x;
}
static_assert(twoReturns(20) == 11 && twoReturns(5) == 5);

// Violations.
constexpr int preBad(int x) pre(x > 0) { return x; } // enforce-error {{contract failed during execution of constexpr function}} observe-warning {{contract failed during execution of constexpr function}} enforce-error {{contract failed during execution of constexpr function}} observe-warning {{contract failed during execution of constexpr function}}
constexpr int bad1 = preBad(0);                      // checked-error {{must be initialized by a constant expression}} checked-note {{in call to 'preBad(0)'}}

constexpr int assertBad(int x) {
  contract_assert(x > 0); // enforce-error {{contract failed during execution of constexpr function}} observe-warning {{contract failed during execution of constexpr function}}
  return x;
}
constexpr int bad2 = assertBad(0); // checked-error {{must be initialized by a constant expression}} checked-note {{in call to 'assertBad(0)'}}

constexpr int postBad(const int x) post(r: r == 2) { return x; } // enforce-error {{contract failed during execution of constexpr function}} observe-warning {{contract failed during execution of constexpr function}}
constexpr int bad3 = postBad(3);                                 // checked-error {{must be initialized by a constant expression}} checked-note {{in call to 'postBad(3)'}}

constexpr const int &refBad(const int &v) post(r: &r != &v) { return v; } // enforce-error {{contract failed during execution of constexpr function}} observe-warning {{contract failed during execution of constexpr function}}
constexpr const int &bad4 = refBad(global);                               // checked-error {{must be initialized by a constant expression}} checked-note {{in call to 'refBad(global)'}}

constexpr S mkBad() post(r: r.self != &r) { return S(); } // enforce-error {{contract failed during execution of constexpr function}} observe-warning {{contract failed during execution of constexpr function}}
constexpr bool bad5 = (mkBad(), true);                    // checked-error {{must be initialized by a constant expression}} checked-note {{in call to 'mkBad()'}}

// An initializer that is constant with the ignore semantic but violates a contract with the checked one is ill-formed
// ([expr.const]); the contracts are evaluated again (the first evaluation ignores them).
const int viaRecheck = preBad(0); // checked-error {{initialization of constant-initialized variable failed}} checked-note {{initializer is constant when evaluated with 'ignore' contract evaluation semantic}} checked-note {{in call to 'preBad(0)'}}


// A variable whose address is taken before its definition keeps its identity when the initializer is evaluated again
// with the contracts enabled.
extern const int declared;
struct Holder {
  const int *p;
};
constexpr Holder holder{&declared};
const int declared = 1;
static_assert(holder.p == &declared);

struct Agg {
  int v;
};
extern const Agg declaredAgg;
constexpr const Agg *aggPtr = &declaredAgg;
const Agg declaredAgg{1};
static_assert(aggPtr == &declaredAgg);

// Variables of class type (and closures) are initialized again too.
Agg agg = Agg();
auto closure = [] { return 1; };
struct WithDefault {
  int a = 2;
};
WithDefault withDefault;

// The contract specifiers may be on the first declaration only.
constexpr int firstDecl(int x) pre(x > 0); // enforce-error {{contract failed during execution of constexpr function}} observe-warning {{contract failed during execution of constexpr function}}
constexpr int firstDecl(int x) { return x; }
constexpr int bad6 = firstDecl(0);           // checked-error {{must be initialized by a constant expression}} checked-note {{in call to 'firstDecl(0)'}}

// Constructors: the preconditions are checked before the member initializers, the postconditions after the body.
struct Ctor {
  int v;
  constexpr Ctor(int x) pre(x > 0) : v(x) {}                                  // enforce-error {{contract failed during execution of constexpr function}} observe-warning {{contract failed during execution of constexpr function}}
  constexpr Ctor(const int x, int) post(v == x) : v(x + 1) {}                  // enforce-error {{contract failed during execution of constexpr function}} observe-warning {{contract failed during execution of constexpr function}}
};
static_assert(Ctor(1).v == 1);
constexpr int bad7 = Ctor(0).v;    // checked-error {{must be initialized by a constant expression}} checked-note {{in call to 'Ctor(0)'}}
constexpr int bad8 = Ctor(1, 0).v; // checked-error {{must be initialized by a constant expression}} checked-note {{in call to 'Ctor(1, 0)'}}
