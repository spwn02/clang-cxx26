//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef TEST_STD_IO_BRIDGE_TEST_BRIDGE_H
#define TEST_STD_IO_BRIDGE_TEST_BRIDGE_H

#include <atomic>
#include <cassert>
#include <coroutine>
#include <exception>
#include <execution>
#include <future>
#include <io>
#include <io_bridge>
#include <stop_token>
#include <system_error>
#include <thread>
#include <utility>

#include "../io/test_io.h"
#include "test_macros.h"

namespace test_bridge {

namespace io = std::io;
namespace ex = std::execution;

// Runs a task on a thread_pool and returns its value (the task must not throw).
template <class T>
T run_on_pool(io::thread_pool& pool, io::task<T> t) {
  std::promise<T> p;
  auto f   = p.get_future();
  auto exe = pool.get_executor();
  io::run_async(exe, [&](T&& v) { p.set_value(std::move(v)); }, [&](std::exception_ptr e) { p.set_exception(e); })(std::move(t));
  return f.get();
}
inline void run_on_pool(io::thread_pool& pool, io::task<void> t) {
  std::promise<void> p;
  auto f   = p.get_future();
  auto exe = pool.get_executor();
  io::run_async(exe, [&] { p.set_value(); }, [&](std::exception_ptr e) { p.set_exception(e); })(std::move(t));
  f.get();
}

// A sender that completes inside start() with either set_value(int) or set_error(error_code). Its error type is
// error_code only, so await_sender returns io_result<int>.
struct ec_sender {
  using sender_concept = ex::sender_tag;
  int value;
  std::error_code ec;
  bool stopped = false;

  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(int), ex::set_error_t(std::error_code), ex::set_stopped_t()>{};
  }
  template <class R>
  struct op {
    using operation_state_concept = ex::operation_state_tag;
    R r;
    int value;
    std::error_code ec;
    bool stopped;
    void start() & noexcept {
      if (stopped)
        ex::set_stopped(std::move(r));
      else if (ec)
        ex::set_error(std::move(r), std::error_code(ec));
      else
        ex::set_value(std::move(r), int(value));
    }
  };
  template <class R>
  op<std::remove_cvref_t<R>> connect(R&& r) const {
    return {std::forward<R>(r), value, ec, stopped};
  }
};

// A sender that never completes by itself: it completes with set_stopped when the stop token of the receiver's
// environment is triggered (from whichever thread requests it).
struct stop_waiting_sender {
  using sender_concept = ex::sender_tag;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(), ex::set_stopped_t()>{};
  }
  template <class R>
  struct op {
    using operation_state_concept = ex::operation_state_tag;
    struct cb {
      op* self;
      void operator()() noexcept { self->arrive(); }
    };
    using token_t = std::stop_token_of_t<ex::env_of_t<R>>;
    R r;
    std::optional<typename token_t::template callback_type<cb>> callback;
    std::atomic<bool> gate{false};
    explicit op(R&& rc) : r(std::move(rc)) {}
    // Completes when both start() has finished registering the callback and the stop request has arrived: the
    // completion may destroy the operation, so it must not happen while start() still touches the object.
    void arrive() noexcept {
      if (gate.exchange(true))
        ex::set_stopped(std::move(r));
    }
    void start() & noexcept {
      callback.emplace(std::get_stop_token(ex::get_env(r)), cb{this});
      arrive();
    }
  };
  template <class R>
  op<std::remove_cvref_t<R>> connect(R&& r) const {
    return op<std::remove_cvref_t<R>>(std::move(r));
  }
};

// An IoAwaitable that suspends until the stop token of its io_env is triggered, then completes (with a void result).
struct wait_for_stop {
  struct cb {
    wait_for_stop* self;
    void operator()() noexcept { self->arrive(); }
  };
  io::continuation c;
  const io::io_env* env = nullptr;
  std::optional<std::stop_callback<cb>> callback;
  std::atomic<bool> gate{false};
  // The coroutine is posted once both await_suspend has finished registering the callback and the stop request has
  // arrived: the resumption may destroy this object.
  void arrive() noexcept {
    if (gate.exchange(true))
      env->executor.post(c);
  }
  bool await_ready() const noexcept { return false; }
  void await_suspend(std::coroutine_handle<> h, const io::io_env* e) {
    c.h = h;
    env = e;
    callback.emplace(e->stop_token, cb{this});
    arrive();
  }
  void await_resume() const noexcept {}
};

} // namespace test_bridge

#endif // TEST_STD_IO_BRIDGE_TEST_BRIDGE_H
