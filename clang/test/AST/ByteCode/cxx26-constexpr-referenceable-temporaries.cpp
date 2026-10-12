// RUN: %clang_cc1 -std=c++26 -fexperimental-new-constant-interpreter -verify=expected,ref %s
// RUN: %clang_cc1 -std=c++26 -verify=expected,classic %s

struct D { int value; constexpr ~D() {} };
constexpr const D *escape() {
  const D &ref = D{9}; // expected-note {{binding reference variable 'ref' here}} expected-note {{temporary created here}}
  return &ref; // expected-warning {{returning address of local temporary object}}
}
constexpr const D *dangling = escape(); // expected-error {{must be initialized by a constant expression}} expected-note {{pointer to temporary is not a constant expression}}

void nested() {
  constexpr const D &ref = D{9}; // ref-note {{temporary created here}} classic-note {{address of non-static constexpr variable 'ref' may differ on each invocation}}
  auto lambda = [&] {
    constexpr const D &bad = ref; // expected-error {{must be initialized by a constant expression}} ref-note {{reference to temporary is not a constant expression}} classic-note {{reference to 'ref' is not a constant expression}}
  };
}
