// RUN: %check_clang_tidy -std=c++29 %s modernize-use-do-expr %t

consteval {
  constexpr int value = 42;
  static_assert(value == 42);
}
// CHECK-MESSAGES-NOT: warning: immediately-invoked lambda

int ordinary_lambda() {
  return [] { return 42; }();
  // CHECK-MESSAGES: :[[@LINE-1]]:10: warning: immediately-invoked lambda can be replaced by a 'do' expression (category: simple) [modernize-use-do-expr]
}
