// RUN: %clang_cc1 -std=c++2d -fsyntax-only -verify %s

// The spelling of a literal token in a token sequence is NUL-terminated and
// owned by the ASTContext: NumericLiteralParser reads one byte past the end
// of a numeric literal. tokenize() copied each literal into the context with
// no terminator (ASTContext allocations are 8 byte aligned), so the copy of
// an 8 character literal was followed directly by the copy of the next one,
// the parser saw the number continue and failed with "failure when lexing a
// numeric literal" (#496, found by compiling the P4380R1 literal testing
// example: tokenize(v, "__uwb") on a 63 bit value, where the byte after the
// literal was whatever the heap held).
// expected-no-diagnostics

#include "../Reflection/Inputs/token-sequence.h"

namespace std::meta {
using info = decltype(^^::);
consteval auto queue_injection(token_sequence) -> void;
template <class... Ts> consteval auto tokenize(Ts const &...) -> token_sequence;
} // namespace std::meta
using std::meta::tokenize;

// Adjacent literals, all of them numeric, the last one followed by nothing.
consteval { std::meta::queue_injection(^^{ constexpr int a[] = { 12, 34, 56 }; }); }
static_assert(a[0] == 12 && a[1] == 34 && a[2] == 56);

// Literals of exactly eight characters leave no padding before the next copy.
consteval {
  auto t = tokenize("{ 12345678, 90123456, 78 }");
  std::meta::queue_injection(^^{ constexpr int e[] = \(t); });
}
static_assert(e[0] == 12345678 && e[1] == 90123456 && e[2] == 78);

// The same through tokenize(): each literal is copied out of the lexer buffer.
consteval {
  auto t = tokenize("{ 12, 34, 0x56, 7.5, 1e2, 78u }");
  std::meta::queue_injection(^^{ constexpr double b[] = \(t); });
}
static_assert(b[0] == 12 && b[1] == 34 && b[2] == 0x56 && b[3] == 7.5 && b[4] == 100 && b[5] == 78);

// A literal whose value comes from a computation, in the form of the paper's
// example: tokenize(value, "ull") injected as an argument.
consteval int answer() { return 42; }
consteval {
  auto t1 = tokenize(answer(), "ull");
  auto t2 = tokenize(answer() + 1, "ull");
  std::meta::queue_injection(^^{ constexpr unsigned long long c[] = { \(t1), \(t2) }; });
}
static_assert(c[0] == 42 && c[1] == 43);

// String and character literals keep their terminator too.
consteval {
  auto t = tokenize("\"ab\" \"cd\"");
  std::meta::queue_injection(^^{ constexpr const char *s = \(t); });
}
static_assert(s[0] == 'a' && s[1] == 'b' && s[2] == 'c' && s[3] == 'd' && s[4] == 0);
