// RUN: clang-check -syntax-tree-dump %s -- -std=c++26 | FileCheck %s

// Declarations the syntax-tree builder used to assert on: an abbreviated
// function template, an explicit object parameter, an attribute on a later
// parameter, a requires-expression.
void abbreviated(auto &&t) {}
struct W {
  int get(this auto &self) { return 1; }
};
void attribute(int v, [[maybe_unused]] int w) {}
struct S {
  int x;
};
static_assert(requires(S s) { s.x; });
template <class T> concept Has = requires(T t) { t.x; };
// CHECK: 'abbreviated'
// CHECK: 'get'
// CHECK: 'attribute'
// CHECK: UnknownExpression
