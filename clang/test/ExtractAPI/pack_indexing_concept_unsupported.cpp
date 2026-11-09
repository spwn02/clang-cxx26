// RUN: rm -rf %t
// RUN: %clang_cc1 -std=c++2d -extract-api --pretty-sgf --emit-sgf-symbol-labels-for-testing \
// RUN:  -triple arm64-apple-macosx -x c++-header %s -o %t.symbols.json -verify

// A pack-index-template-name as a type-constraint is not implemented (P3670R4);
// the template parameter then has no type constraint, which ExtractAPI must
// cope with after the error.
template <template <class> concept... CC, CC...[0] T>
void function(T);
// expected-error@-2 {{a pack-index-template-name as a type-constraint is not implemented yet}}
