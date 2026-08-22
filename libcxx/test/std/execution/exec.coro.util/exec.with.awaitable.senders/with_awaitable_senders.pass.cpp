//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <execution>

// template<class-type Promise>
// struct with_awaitable_senders { ... };

#include <cassert>
#include <coroutine>
#include <exception>
#include <execution>
#include <utility>

using namespace std::execution;

// A minimal eager coroutine whose promise derives from with_awaitable_senders, so `co_await`
// on a sender is routed through the inherited await_transform -> as_awaitable.
struct FinalAwaiter {
  bool await_ready() noexcept { return false; }
  template <class Promise>
  std::coroutine_handle<> await_suspend(std::coroutine_handle<Promise> h) noexcept {
    auto c = h.promise().continuation();
    return c ? c : std::noop_coroutine();
  }
  void await_resume() noexcept {}
};

struct Task {
  struct promise_type : with_awaitable_senders<promise_type> {
    Task get_return_object() { return Task{std::coroutine_handle<promise_type>::from_promise(*this)}; }
    std::suspend_always initial_suspend() noexcept { return {}; }
    FinalAwaiter final_suspend() noexcept { return {}; }
    void return_value(int v) noexcept { result = v; }
    void unhandled_exception() noexcept { error = std::current_exception(); }

    int result = 0;
    std::exception_ptr error;
  };

  std::coroutine_handle<promise_type> h;
  ~Task() {
    if (h)
      h.destroy();
  }
};

Task value_task() { co_return co_await just(42); }

Task error_task() {
  try {
    co_await just_error(std::runtime_error("boom"));
    co_return 0; // unreachable
  } catch (std::runtime_error const&) {
    co_return -1;
  }
}

Task stopped_task() {
  co_await just_stopped();
  co_return 0; // unreachable if stopped correctly interrupts
}

// A second, independent coroutine type used purely as the downstream continuation target for
// set_continuation -- its promise supplies its own unhandled_stopped(), overriding
// with_awaitable_senders' default (terminating) handler once wired up.
struct Sink {
  struct promise_type {
    Sink get_return_object() { return Sink{std::coroutine_handle<promise_type>::from_promise(*this)}; }
    std::suspend_always initial_suspend() noexcept { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void return_void() noexcept {}
    void unhandled_exception() noexcept {}
    std::coroutine_handle<> unhandled_stopped() noexcept {
      if (flag)
        *flag = true;
      return std::noop_coroutine();
    }
    bool* flag = nullptr;
  };

  std::coroutine_handle<promise_type> h;
  ~Sink() {
    if (h)
      h.destroy();
  }
};

Sink sink_coro() { co_return; }

int main(int, char**) {
  // Value propagation through the inherited await_transform.
  {
    Task t = value_task();
    t.h.resume();
    assert(t.h.done());
    assert(t.h.promise().result == 42);
  }
  // Error propagation: sender-awaitable rethrows from await_resume, observable as a thrown
  // exception at the co_await expression.
  {
    Task t = error_task();
    t.h.resume();
    assert(t.h.done());
    assert(t.h.promise().result == -1);
  }
  // Stopped propagation without set_continuation: unhandled_stopped() falls back to the
  // default handler. Not exercised here (it terminates), only its override below is.

  // Stopped propagation with set_continuation: unhandled_stopped() routes to the downstream
  // promise's own unhandled_stopped, and the awaiting coroutine is never resumed past the
  // co_await point (per [exec.with.awaitable.senders]p1's note).
  {
    bool stopped_observed = false;
    Sink sink              = sink_coro();
    sink.h.promise().flag = &stopped_observed;

    Task t = stopped_task();
    assert(t.h.promise().continuation() == std::coroutine_handle<>{});
    t.h.promise().set_continuation(sink.h);
    assert(t.h.promise().continuation() == sink.h);

    t.h.resume();
    assert(stopped_observed);
    // The coroutine was never resumed to completion: set_stopped short-circuits past
    // final_suspend entirely.
    assert(!t.h.done());
  }
  return 0;
}
