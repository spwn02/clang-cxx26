//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: no-threads

// <execution>

// namespace execution {
//   template <class T = void, class Environment = void>
//   class task { ... };
//   template <class Err>
//   struct with_error { ... };
//   template <class Sch>
//   struct change_coroutine_scheduler { ... };
// }
//
// This pass's scope cuts (see docs/design/execution_task_p3552.md): every task<T, Environment>
// completes with exactly set_error_t(exception_ptr) regardless of Environment (a custom
// Environment::error_types is not supported), and affine_on's same-resource fast path (an
// allowed optimization per the adopted wording) is not implemented -- every scheduling hop is
// taken unconditionally. Both are exercised indirectly below (every error path still works;
// scheduling still completes correctly, just always through one extra hop).

#include <cassert>
#include <coroutine>
#include <execution>
#include <memory>
#include <stdexcept>
#include <utility>

using namespace std::execution;

task<int> returns_value() { co_return 42; }

task<void> awaits_inner_sender() {
  co_await just();
  co_return;
}

task<int> throws_exception() { throw std::runtime_error("boom"); }

task<int> yields_error() {
  co_yield with_error<std::runtime_error>(std::runtime_error("yielded"));
  co_return 0; // unreachable
}

task<int> changes_scheduler() {
  co_await change_coroutine_scheduler<task_scheduler>{task_scheduler(inline_scheduler{})};
  co_return 7;
}

task<int> gets_stopped() {
  co_await just_stopped();
  co_return 123; // unreachable
}

task<std::unique_ptr<int>> returns_move_only() { co_return std::make_unique<int>(5); }

struct value_rcvr {
  using receiver_concept = receiver_tag;
  int* out;

  void set_value(int v) && noexcept { *out = v; }
  void set_error(std::exception_ptr) && noexcept { assert(false); }
  void set_stopped() && noexcept { assert(false); }
  auto get_env() const noexcept { return env<>{}; }
};

int main(int, char**) {
  // co_return with a value, consumed through sync_wait (task is an ordinary sender).
  {
    auto result = std::this_thread::sync_wait(returns_value());
    assert(result.has_value());
    assert(std::get<0>(*result) == 42);
  }

  // task<void>, co_await-ing a plain sender inside the body.
  {
    auto result = std::this_thread::sync_wait(awaits_inner_sender());
    assert(result.has_value());
  }

  // An uncaught exception propagates as a set_error(exception_ptr) completion.
  {
    bool caught = false;
    try {
      (void)std::this_thread::sync_wait(throws_exception());
    } catch (const std::runtime_error&) {
      caught = true;
    }
    assert(caught);
  }

  // co_yield with_error<Err>{e} reports an error completion without an actual throw.
  {
    bool caught = false;
    try {
      (void)std::this_thread::sync_wait(yields_error());
    } catch (const std::runtime_error&) {
      caught = true;
    }
    assert(caught);
  }

  // co_await change_coroutine_scheduler{sch} switches schedulers mid-body and execution
  // continues correctly afterward.
  {
    auto result = std::this_thread::sync_wait(changes_scheduler());
    assert(result.has_value());
    assert(std::get<0>(*result) == 7);
  }

  // A co_await-ed sender that completes with set_stopped short-circuits the rest of the body
  // and delivers set_stopped directly to whatever the task is connected to.
  {
    auto result = std::this_thread::sync_wait(gets_stopped());
    assert(!result.has_value());
  }

  // Move-only value types flow through task<T> correctly.
  {
    auto result = std::this_thread::sync_wait(returns_move_only());
    assert(result.has_value());
    assert(*std::get<0>(*result) == 5);
  }

  // Direct connect()/start(), not just via sync_wait.
  {
    int out = 0;
    auto op = std::move(returns_value()).connect(value_rcvr{&out});
    start(op);
    assert(out == 42);
  }

  return 0;
}
