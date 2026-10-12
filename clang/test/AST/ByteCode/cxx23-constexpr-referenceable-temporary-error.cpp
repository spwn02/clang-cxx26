// RUN: %clang_cc1 -std=c++20 -fsyntax-only -verify=expected,init %s
// RUN: %clang_cc1 -std=c++20 -fsyntax-only -fexperimental-new-constant-interpreter -verify=expected,init %s
// RUN: %clang_cc1 -std=c++23 -fsyntax-only -verify=expected,classic %s
// RUN: %clang_cc1 -std=c++23 -fsyntax-only -fexperimental-new-constant-interpreter -verify=expected,init %s

// Before P2686R5 a constexpr reference to a lifetime-extended automatic
// temporary is ill-formed. The bytecode interpreter must diagnose the later
// use of the reference instead of dereferencing the dangling storage.
constexpr int temporary() {
  struct D { int value; constexpr ~D() {} };
  constexpr const D &ref = D{9}; // #decl \
  // expected-error {{constexpr variable 'ref' must be initialized by a constant expression}} \
  // expected-note {{reference to temporary is not a constant expression}} \
  // expected-note {{temporary created here}}
  static_assert(ref.value == 9); // expected-error {{static assertion expression is not an integral constant expression}} \
  // classic-note {{read of variable 'ref' whose value is not known}} \
  // init-note {{initializer of 'ref' is not a constant expression}}
  // expected-note@#decl {{declared here}}
  return ref.value;
}

// A reference to an object with static storage duration is fine.
void statics() {
  static constexpr int s = 1;
  constexpr const int &r = s;
  static_assert(r == 1);
  static constexpr struct S { int v; } t{3};
  constexpr const S &rt = t;
  static_assert(rt.v == 3);
}
