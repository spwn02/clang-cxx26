// RUN: clang-tidy -checks='-*,bugprone-unchecked-optional-access,bugprone-use-after-move' %s -- -std=c++2d -I %S/../../../../clang/test/Reflection/Inputs 2>&1 | FileCheck --allow-empty %s
// CHECK-NOT: Assertion
// CHECK-NOT: Stack dump

// The dataflow framework records a result object for every record prvalue: a
// token sequence literal passed to a function is one, and the expressions
// interpolated into it are not part of it.
#include "token-sequence.h"

namespace std::meta {
using info = decltype(^^::);
template <class T> consteval info reflect_constant(const T &);
} // namespace std::meta

consteval int take(std::meta::token_sequence) { return 1; }

__macro pass(std::meta::token_sequence t) { return t; }

consteval int use(int n) {
  int v = take(^^{ \(std::meta::reflect_constant(n)) });
  return v;
}

int runtime(int k) {
  pass!(k);
  return k;
}
