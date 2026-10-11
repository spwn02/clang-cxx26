// RUN: %clang_cc1 -std=c++26 -freflection -fsyntax-only -verify %s
// expected-no-diagnostics

namespace std::meta { using info = decltype(^^int); }

consteval int *allocate() { return new int(42); }
consteval std::meta::info consume(int *p) {
  delete p;
  return ^^int;
}

// The outer call handles a consteval-only value, but still prevents its
// nested allocation from being evaluated as a separate immediate invocation.
constexpr auto reflected = consume(allocate());
static_assert(reflected == ^^int);
struct Static {
  static constexpr auto reflected = consume(allocate());
};
static_assert(Static::reflected == ^^int);
