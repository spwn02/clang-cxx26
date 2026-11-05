// RUN: clang-check -syntax-tree-dump %s -- -std=c++26 | FileCheck %s

// A consteval block, a reflection of the global namespace and a concept: the
// syntax tree builder used to assert on each.
consteval { }
constexpr auto global = ^^::;
template <class T> concept Always = true;
// CHECK: UnknownDeclaration
// CHECK: 'global'
// CHECK: UnknownDeclaration
