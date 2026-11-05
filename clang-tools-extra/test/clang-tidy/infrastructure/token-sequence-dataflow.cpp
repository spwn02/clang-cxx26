// RUN: clang-tidy -checks='-*,bugprone-unchecked-optional-access' %s -- -std=c++2d -I %S/../../../../clang/test/Reflection/Inputs 2>&1 | FileCheck --allow-empty %s
// CHECK-NOT: Assertion
// CHECK-NOT: Stack dump

// The dataflow framework records a result object for every record prvalue: a
// token sequence literal passed to a function is one, and the expressions
// interpolated into it are not part of it.
#include <optional>
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

// An interpolated token sequence is a record glvalue inside the literal; the
// function is analyzed because it uses an optional.
consteval int scaled(int factor, std::meta::token_sequence out, std::optional<int> o) {
  int r = take(^^{ \(factor) * (\(out)) });
  return o ? r : 0;
}

int runtime(int k) {
  pass!(k);
  return k;
}
