// RUN: not %clang -std=c++29 -nostdinc++ -isystem %S/../../../libcxx/include -idirafter %S/../../../build-libcxx/include/c++/v1 -fsyntax-only %s 2>&1 | FileCheck %s

// P2758R5: a std::constexpr_error_str call is constexpr-erroneous but evaluation
// continues (the expression stays a core constant expression), so every message
// of one macro body is issued; the expansion then fails and produces no tokens.
#include <debugging>
#include <meta>
using std::meta::token_sequence;

__macro two_messages(token_sequence ts) {
  std::constexpr_error_str("first", "one");
  std::constexpr_error_str("second", "two");
  return ^^{ 0 };
}
int x = two_messages!(a);

// One error at the invocation, then both messages as notes in order.
// CHECK: expression-macro-error-continues.cpp:15:{{[0-9]+}}: error: expression macro 'two_messages' reported an error
// CHECK: note: constexpr message with tag 'first': one
// CHECK: note: constexpr message with tag 'second': two
// CHECK: 1 error generated.
