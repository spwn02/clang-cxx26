// RUN: %check_clang_tidy -std=c++29 %s readability-else-after-return %t -- -- -fdo-expressions

int f(bool b, int n) {
  return do {
    if (b) {
      do_return 1;
    } else if (n > 2) {
    // CHECK-MESSAGES: :[[@LINE-1]]:7: warning: do not use 'else' after 'do_return' [readability-else-after-return]
    // CHECK-FIXES: {{^}}    } if (n > 2) {{{$}}
      do_return 2;
    }
    do_return 3;
  };
}
