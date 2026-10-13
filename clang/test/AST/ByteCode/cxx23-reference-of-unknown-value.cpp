// RUN: %clang_cc1 -std=c++20 -fsyntax-only -verify=cxx20 %s
// RUN: %clang_cc1 -std=c++20 -fsyntax-only -fexperimental-new-constant-interpreter -verify=cxx20 %s
// RUN: %clang_cc1 -std=c++23 -fsyntax-only -verify=cxx23 %s
// RUN: %clang_cc1 -std=c++23 -fsyntax-only -fexperimental-new-constant-interpreter -verify=cxx23 %s
// RUN: %clang_cc1 -std=c++2c -fsyntax-only -verify=cxx26 %s
// RUN: %clang_cc1 -std=c++2c -fsyntax-only -fexperimental-new-constant-interpreter -verify=cxx26 %s

// P2280R4 (C++23): a reference whose referent is unknown can be used in a
// constant expression as long as the object is not read.
template <class T, unsigned N> constexpr unsigned size(T (&)[N]) { return N; }

constexpr int type_only() {
  int local[3]{};
  int (&ref)[3] = local; // #ref1
  static_assert(size(ref) == 3); // cxx20-error {{static assertion expression is not an integral constant expression}} \
  // cxx20-note {{initializer of 'ref' is not a constant expression}}
  // cxx20-note@#ref1 {{declared here}}
  return 0;
}
static_assert(type_only() == 0);

constexpr int reads_unknown() {
  int local = 3; // #local
  int &ref = local; // #ref2
  static_assert(ref == 3); // cxx20-error {{static assertion expression is not an integral constant expression}} \
  // cxx20-note {{initializer of 'ref' is not a constant expression}} \
  // cxx23-error {{static assertion expression is not an integral constant expression}} \
  // cxx23-note {{read of non-constexpr variable 'ref' is not allowed in a constant expression}} \
  // cxx26-error {{static assertion expression is not an integral constant expression}} \
  // cxx26-note {{read of non-const variable 'local' is not allowed in a constant expression}}
  // cxx20-note@#ref2 {{declared here}}
  // cxx23-note@#ref2 {{declared here}}
  // cxx26-note@#local {{declared here}}
  return ref;
}
