// RUN: clang-check -syntax-tree-dump %s -- -std=c++2d -I %S/../Reflection/Inputs | FileCheck %s

// Every form of macro invocation and of injection the syntax-tree builder is
// given: it builds a tree (the invocation is one leaf; what was expanded from a
// literal is not written where it is used).
#include "token-sequence.h"

namespace std::meta {
using info = decltype(^^::);
consteval auto queue_injection(token_sequence) -> void;
consteval auto queue_injection(info, token_sequence) -> void;
}
using std::meta::queue_injection;

__macro inc(int x) { return ^^{ (\(x) + 1) }; }
__macro keep(std::meta::token_sequence t) { return t; }
__macro declare_it(std::meta::token_sequence name) { return ^^{ int \(name) = 1; }; }
__macro make_box(std::meta::token_sequence n) {
  return ^^{ template <class T> struct \(n) { T v; }; };
}
struct Holder {
  int v;
  __macro bump(this Holder &self, int n) { return ^^{ (\(self).v += \(n)) }; }
};

declare_it!(zz);
make_box!(Box);
namespace Target {}
consteval { queue_injection(^^Target, ^^{ int targeted(int a, int b) { return a + b; } }); }

struct C {
  int m;
  C() : keep!(m(3)) {}
};

template <class T> int tmpl(T t) { return inc!(t); }
int use(Holder h, int k) {
  consteval { queue_injection(^^{ int local = 1; }); }
  h.bump!(4);
  inc!(k);
  return inc!(k) + h.bump!(1);
}
// CHECK: TranslationUnit
// CHECK: 'use'
// CHECK: UnknownExpression
