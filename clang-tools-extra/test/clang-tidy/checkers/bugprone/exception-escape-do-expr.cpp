// RUN: %check_clang_tidy -std=c++29 %s bugprone-exception-escape %t -- -- -fexceptions

int throws_in_do_body() noexcept {
  // CHECK-MESSAGES: :[[@LINE-1]]:5: warning: an exception may be thrown in function 'throws_in_do_body' which should not throw exceptions
  return do {
    throw 1;
    do_return 0;
  };
}

int throws_on_one_path(bool b) noexcept {
  // CHECK-MESSAGES: :[[@LINE-1]]:5: warning: an exception may be thrown in function 'throws_on_one_path' which should not throw exceptions
  return do {
    if (b)
      throw 1;
    do_return 0;
  };
}

int caught_inside(bool b) noexcept {
  return do {
    try {
      if (b)
        throw 1;
    } catch (...) {
    }
    do_return 0;
  };
}
