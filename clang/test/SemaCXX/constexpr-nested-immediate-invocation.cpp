// RUN: %clang_cc1 -std=c++23 -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++23 -fsyntax-only -verify -fexperimental-new-constant-interpreter %s
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify -fexperimental-new-constant-interpreter %s
// expected-no-diagnostics

// An allocation can escape a nested immediate call if the enclosing call
// releases it. The nested call is not an independent immediate invocation.
consteval int *allocate() { return new int(42); }
consteval int consume(int *p) {
  int value = *p;
  delete p;
  return value;
}

constexpr int answer = consume(allocate());
static_assert(answer == 42);
struct Static {
  static constexpr int answer = consume(allocate());
};
static_assert(Static::answer == 42);

// Checking the inner allocation in isolation must not escalate a constexpr
// function template. Its initializer is evaluated as a whole instead.
template <typename T> constexpr int add(T value) {
  constexpr int answer = consume(allocate());
  return value + answer;
}
static_assert(add(0) == 42);
int runtime(int value) { return add(value); }
