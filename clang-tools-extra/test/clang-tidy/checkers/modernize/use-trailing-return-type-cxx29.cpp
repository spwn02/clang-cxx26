// RUN: %check_clang_tidy -std=c++29 %s modernize-use-trailing-return-type %t

int contracted(const int value) pre(value > 0) post(result: result > value) {
  return value + 1;
}
// CHECK-MESSAGES-NOT: warning: use a trailing return type for this function

int ordinary(int value) { return value; }
// CHECK-MESSAGES: :[[@LINE-1]]:5: warning: use a trailing return type for this function [modernize-use-trailing-return-type]
// CHECK-FIXES: auto ordinary(int value) -> int { return value; }
