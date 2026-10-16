//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <execution>

// [exec.connect]: the operation state of connecting an awaitable (operation-state-task) is not movable.

#include <cassert>
#include <coroutine>
#include <execution>
#include <type_traits>
#include <utility>

namespace ex = std::execution;

struct Awaiter {
  bool await_ready() noexcept { return true; }
  void await_suspend(std::coroutine_handle<>) noexcept {}
  int await_resume() { return 42; }
};

struct Rcvr {
  using receiver_concept = ex::receiver_tag;
  int* count = nullptr;
  void set_value(int v) && noexcept { *count += v; }
  void set_error(std::exception_ptr) && noexcept { *count += 1000; }
  void set_stopped() && noexcept { *count += 100000; }
};

void test_operation_state() {
  int n = 0;
  auto op = ex::connect(Awaiter{}, Rcvr{&n});
  static_assert(!std::is_move_constructible_v<decltype(op)>);
  static_assert(!std::is_copy_constructible_v<decltype(op)>);
  ex::start(op);
  assert(n == 42);
}

int main(int, char**) {
  test_operation_state();
  return 0;
}
