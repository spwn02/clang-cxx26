// RUN: %clang_cc1 -std=c++20 -fsyntax-only -verify=cxx20 %s
// RUN: %clang_cc1 -std=c++20 -fsyntax-only -fexperimental-new-constant-interpreter -verify=cxx20 %s
// RUN: %clang_cc1 -std=c++23 -fsyntax-only -verify=cxx23 %s
// RUN: %clang_cc1 -std=c++23 -fsyntax-only -fexperimental-new-constant-interpreter -verify=cxx23 %s
// RUN: %clang_cc1 -std=c++2c -fsyntax-only -verify=cxx26 %s
// RUN: %clang_cc1 -std=c++2c -fsyntax-only -fexperimental-new-constant-interpreter -verify=cxx26 %s

// P2686R5 (C++26): inside a constexpr function a reference initialized with a
// constant expression can be used in a constant expression, also when it is not
// constexpr and binds a lifetime-extended temporary. Before C++26 it cannot.
// Both interpreters must agree.

struct D { int value; constexpr ~D() {} };
struct E { int value; };

constexpr int class_temp() {
  const D &ref = D{9};
  static_assert(ref.value == 9); // cxx20-error {{static assertion expression is not an integral constant expression}} cxx23-error {{static assertion expression is not an integral constant expression}}
  return ref.value;
}

constexpr int binding() {
  const auto &[x] = E{9};
  static_assert(x == 9); // cxx20-error {{static assertion expression is not an integral constant expression}} cxx23-error {{static assertion expression is not an integral constant expression}}
  return x;
}

constexpr int scalar_temp() {
  const int &r = 9;
  static_assert(r == 9); // cxx20-error {{static assertion expression is not an integral constant expression}} cxx23-error {{static assertion expression is not an integral constant expression}}
  return r;
}

constexpr int through_reference() {
  const int &r = 9;
  const int &s = r;
  static_assert(s == 9); // cxx20-error {{static assertion expression is not an integral constant expression}} cxx23-error {{static assertion expression is not an integral constant expression}}
  return s;
}

// Reads that stay ill-formed in every mode.
constexpr int to_non_constant_object() {
  int a = 3;
  const int &r = a;
  static_assert(r == 3); // cxx20-error {{static assertion expression is not an integral constant expression}} cxx23-error {{static assertion expression is not an integral constant expression}} cxx26-error {{static assertion expression is not an integral constant expression}}
  return r;
}

constexpr int rvalue_reference_to_non_const() {
  E &&r = E{9};
  static_assert(r.value == 9); // cxx20-error {{static assertion expression is not an integral constant expression}} cxx23-error {{static assertion expression is not an integral constant expression}} cxx26-error {{static assertion expression is not an integral constant expression}}
  return r.value;
}

// The functions themselves are fine to evaluate in every mode.
static_assert(scalar_temp() == 9);
static_assert(class_temp() == 9);

// cxx20-note@* 0+ {{}}
// cxx23-note@* 0+ {{}}
// cxx26-note@* 0+ {{}}
