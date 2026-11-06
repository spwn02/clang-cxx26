// RUN: %clang_cc1 -std=c++29 -I %S/../Reflection/Inputs -emit-pch -x c++-header -o %t.pch %S/Inputs/token-sequence-instantiation.h
// RUN: %clang_cc1 -std=c++29 -I %S/../Reflection/Inputs -include-pch %t.pch -fsyntax-only -verify %s
// expected-no-diagnostics

// A token sequence literal instantiated from a template, with the class of its
// type read from a PCH (an editor's preamble): the class loads its fields
// lazily, and its hidden member is what makes it consteval-only.
consteval int length(std::meta::token_sequence) { return 1; }
template <class T> consteval int make() {
  int n = length(^^{ x });
  return n;
}
constexpr int ts = make<int>();
