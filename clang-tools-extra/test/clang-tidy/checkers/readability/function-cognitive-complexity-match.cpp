// RUN: %check_clang_tidy -std=c++2d %s readability-function-cognitive-complexity %t -- -config='{CheckOptions: {readability-function-cognitive-complexity.Threshold: 0}}'

// A match selection counts like a switch: one for the selection, nesting
// penalized, and what is nested in it is one level deeper.

int one_match(int x) {
  // CHECK-MESSAGES: :[[@LINE-1]]:5: warning: function 'one_match' has cognitive complexity of 1 (threshold 0) [readability-function-cognitive-complexity]
  return match (x) {
    // CHECK-MESSAGES: :[[@LINE-1]]:10: note: +1, including nesting penalty of 0, nesting level increased to 1
    case 0 => 1;
    case _ => 2;
  };
}

int nested(int x, int y) {
  // CHECK-MESSAGES: :[[@LINE-1]]:5: warning: function 'nested' has cognitive complexity of 3 (threshold 0) [readability-function-cognitive-complexity]
  return match (x) {
    // CHECK-MESSAGES: :[[@LINE-1]]:10: note: +1, including nesting penalty of 0, nesting level increased to 1
    case 0 => y ? 1 : 2;
    // CHECK-MESSAGES: :[[@LINE-1]]:17: note: +2, including nesting penalty of 1, nesting level increased to 2
    case _ => 3;
  };
}
