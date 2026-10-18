// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

// The correction of a name looked up in a class is the namespace of an
// enclosing scope ('utd' -> 'std'): the nested-name-specifier cannot name a
// namespace after a class, so the correction replaces the whole specifier
// (llvm_unreachable "invalid prefix for namespace" in NestedNameSpecifier).
namespace std { struct S {}; }
int x = std::S::utd::b;
// expected-error@-1 {{no member named 'utd' in 'std::S'; did you mean 'std'?}}
// expected-error@-2 {{no member named 'b' in namespace 'std'}}
// expected-note@7 {{'std' declared here}}
