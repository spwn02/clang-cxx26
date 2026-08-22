//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <execution>

// connect(sndr, rcvr) falls back to the exposition-only connect-awaitable(new_sndr, rcvr)
// -- [exec.connect]p6.2 -- when new_sndr has no member connect() but is awaitable. This
// exercises that fallback end to end: a bare, non-sender awaitable (no sender_concept, no
// member connect()) is a `sender` only via enable-sender's awaitable disjunct
// ([exec.snd.concepts]), and connecting it must actually produce a working operation state.

#include <cassert>
#include <coroutine>
#include <exception>
#include <execution>
#include <stdexcept>
#include <utility>

using namespace std::execution;

// Resumes its coroutine handle synchronously from within await_suspend, so the whole
// connect-awaitable coroutine runs to completion (and delivers to the receiver) inside the
// single call to op.start() -- no real scheduler is needed for this test.
struct ValueAwaitable {
  int value;
  bool await_ready() noexcept { return false; }
  void await_suspend(std::coroutine_handle<> h) noexcept { h.resume(); }
  int await_resume() noexcept { return value; }
};

struct VoidAwaitable {
  bool await_ready() noexcept { return false; }
  void await_suspend(std::coroutine_handle<> h) noexcept { h.resume(); }
  void await_resume() noexcept {}
};

struct ThrowingAwaitable {
  bool await_ready() noexcept { return false; }
  void await_suspend(std::coroutine_handle<> h) noexcept { h.resume(); }
  int await_resume() { throw std::runtime_error("boom"); }
};

// Routes through connect-awaitable-promise::unhandled_stopped() -- the third, otherwise
// untouched, channel of connect-awaitable's completion surface (value/error are exercised
// above; nothing else in this file instantiates unhandled_stopped()). `__is_awaitable`
// accepts this: GET-AWAITER's promise-probe is `__env_promise<env<>>`, which itself declares
// unhandled_stopped() (awaitable.h), so the templated await_suspend below is satisfiable
// even for that probe promise, without ever calling it there.
//
// await_resume()'s `int` return (never actually reached -- unhandled_stopped() returns
// noop_coroutine(), so the coroutine never resumes past this await-point) exists only so
// this awaitable's await-result-type matches MyReceiver's set_value(int): connect-awaitable's
// own receiver_of<DR, Sigs> Mandate is a purely structural, compile-time check against the
// await-expression's static result type, independent of which branch actually runs.
struct StoppedAwaitable {
  bool await_ready() noexcept { return false; }
  template <class _Promise>
  std::coroutine_handle<> await_suspend(std::coroutine_handle<_Promise> h) noexcept {
    return h.promise().unhandled_stopped();
  }
  int await_resume() noexcept { return -1; }
};

static_assert(sender<ValueAwaitable>);
static_assert(sender<VoidAwaitable>);
static_assert(sender<StoppedAwaitable>);

struct Result {
  int value      = -1;
  bool has_error = false;
  bool stopped   = false;
};

struct MyReceiver {
  using receiver_concept = receiver_tag;
  Result* result;
  void set_value(int v) && noexcept { result->value = v; }
  void set_error(std::exception_ptr) && noexcept { result->has_error = true; }
  void set_stopped() && noexcept { result->stopped = true; }
  auto get_env() const noexcept { return env<>{}; }
};

struct VoidReceiver {
  using receiver_concept = receiver_tag;
  Result* result;
  void set_value() && noexcept { result->value = 0; }
  void set_error(std::exception_ptr) && noexcept { result->has_error = true; }
  void set_stopped() && noexcept { result->stopped = true; }
  auto get_env() const noexcept { return env<>{}; }
};

int main(int, char**) {
  // Value completion, driven entirely through connect-awaitable's synchronous-resume path.
  {
    Result r;
    auto op = connect(ValueAwaitable{42}, MyReceiver{&r});
    start(op);
    assert(r.value == 42);
    assert(!r.has_error);
    assert(!r.stopped);
  }
  // Void completion: the co_await-then-suspend-complete(set_value) branch (no datum).
  {
    Result r;
    auto op = connect(VoidAwaitable{}, VoidReceiver{&r});
    start(op);
    assert(r.value == 0);
    assert(!r.has_error);
    assert(!r.stopped);
  }
  // An exception thrown out of await_resume is caught by connect-awaitable's try/catch and
  // delivered via suspend-complete(set_error, ..., exception_ptr) -- and only that: the
  // set_value awaiter's await_resume is unreachable, so set_value must never also fire.
  {
    Result r;
    auto op = connect(ThrowingAwaitable{}, MyReceiver{&r});
    start(op);
    assert(r.value == -1);
    assert(r.has_error);
    assert(!r.stopped);
  }
  // unhandled_stopped(): set_stopped(rcvr) fires and the coroutine is destroyed cleanly by
  // the operation state's destructor without ever reaching final_suspend's terminate().
  {
    Result r;
    auto op = connect(StoppedAwaitable{}, MyReceiver{&r});
    start(op);
    assert(r.value == -1);
    assert(!r.has_error);
    assert(r.stopped);
  }
  return 0;
}
