//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <execution>

// struct as_awaitable_t { ... };
// inline constexpr as_awaitable_t as_awaitable{};

#include <cassert>
#include <coroutine>
#include <exception>
#include <execution>
#include <utility>

using namespace std::execution;

// A minimal coroutine promise that routes co_await through as_awaitable directly (not
// through with_awaitable_senders, which is tested separately) -- exercises
// [exec.as.awaitable]p7's dispatch end to end: (7.3) for an already-awaitable object passed
// straight through, and (7.4) for a sender wrapped via sender-awaitable.
struct PlainAwaiter {
  bool await_ready() noexcept { return true; }
  void await_suspend(std::coroutine_handle<>) noexcept {}
  int await_resume() noexcept { return 11; }
};

template <class T>
struct Task {
  struct promise_type {
    Task get_return_object() { return Task{std::coroutine_handle<promise_type>::from_promise(*this)}; }
    std::suspend_always initial_suspend() noexcept { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void return_value(T v) noexcept { result = std::move(v); }
    void unhandled_exception() noexcept { error = std::current_exception(); }

    template <class Value>
    decltype(auto) await_transform(Value&& value) {
      return as_awaitable(std::forward<Value>(value), *this);
    }

    T result{};
    std::exception_ptr error;
  };

  std::coroutine_handle<promise_type> h;
  ~Task() {
    if (h)
      h.destroy();
  }
};

Task<int> uses_plain_awaiter() { co_return co_await PlainAwaiter{}; }

Task<int> uses_sender() { co_return co_await just(42); }

Task<int> uses_sender_chain() { co_return co_await (just(10) | then([](int x) { return x * 2; })); }

Task<int> propagates_error() {
  try {
    co_await just_error(std::runtime_error("boom"));
    co_return 0; // unreachable
  } catch (std::runtime_error const&) {
    co_return -1;
  }
}

int main(int, char**) {
  // (7.3): PlainAwaiter is already an awaiter -- passed straight through.
  {
    Task<int> t = uses_plain_awaiter();
    t.h.resume();
    assert(t.h.done());
    assert(t.h.promise().result == 11);
  }
  // (7.4): a single-valued sender is wrapped in sender-awaitable.
  {
    Task<int> t = uses_sender();
    t.h.resume();
    assert(t.h.done());
    assert(t.h.promise().result == 42);
  }
  {
    Task<int> t = uses_sender_chain();
    t.h.resume();
    assert(t.h.done());
    assert(t.h.promise().result == 20);
  }
  // sender-awaitable's set_error path: the error is rethrown from await_resume, observable
  // as a thrown exception at the co_await expression.
  {
    Task<int> t = propagates_error();
    t.h.resume();
    assert(t.h.done());
    assert(t.h.promise().result == -1);
  }
  return 0;
}
