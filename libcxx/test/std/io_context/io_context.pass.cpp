//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23, c++26
// UNSUPPORTED: no-threads
// REQUIRES: linux || darwin || freebsd || windows
// ADDITIONAL_COMPILE_FLAGS: -fexperimental-library

// <io_context>

// std::io::io_context: run/run_one/poll/stop/restart/stopped, work counting, dispatch and post, tasks started with
// run_async on its executor, posting from other threads.

#include <io_context>

#include <atomic>
#include <cassert>
#include <thread>
#include <vector>

#include "test_ctx.h"
#include "test_macros.h"

using namespace test_ctx;

io::task<void> bump(std::atomic<int>& n, std::thread::id* where = nullptr) {
  ++n;
  if (where)
    *where = std::this_thread::get_id();
  co_return;
}

// Hops through the executor several times.
struct post_op {
  io::continuation c;
  bool await_ready() const noexcept { return false; }
  void await_suspend(std::coroutine_handle<> h, const io::io_env* env) {
    c.h = h;
    env->executor.post(c);
  }
  void await_resume() const noexcept {}
};
io::task<void> hopper(std::atomic<int>& n, int hops) {
  for (int i = 0; i != hops; ++i)
    co_await post_op{};
  ++n;
}

int main(int, char**) {
  static_assert(io::ExecutionContext<io::io_context>);
  static_assert(io::Executor<io::io_context::executor_type>);

  // An idle context: run() returns 0 at once and leaves the context stopped; restart() makes it runnable again.
  {
    io::io_context ctx;
    assert(!ctx.stopped());
    assert(ctx.run() == 0);
    assert(ctx.stopped());
    ctx.restart();
    assert(!ctx.stopped());
    assert(ctx.poll() == 0);
  }

  // A task started on the executor runs on the thread that calls run().
  {
    io::io_context ctx;
    std::atomic<int> n{0};
    std::thread::id where;
    io::run_async(ctx.get_executor())(bump(n, &where));
    assert(n == 0); // nothing runs before run()
    assert(ctx.run() == 1);
    assert(n == 1);
    assert(where == std::this_thread::get_id());
    assert(ctx.stopped());
  }

  // run_one() and poll_one() resume one continuation at a time.
  {
    io::io_context ctx;
    std::atomic<int> n{0};
    for (int i = 0; i != 3; ++i)
      io::run_async(ctx.get_executor())(bump(n));
    assert(ctx.run_one() == 1);
    assert(n == 1);
    assert(ctx.poll_one() == 1);
    assert(n == 2);
    assert(ctx.poll() == 1);
    assert(n == 3);
    assert(ctx.poll() == 0);
  }

  // stop() ends run() while work is left; restart() and run() continue.
  {
    io::io_context ctx;
    std::atomic<int> n{0};
    io::run_async(ctx.get_executor())(hopper(n, 1000));
    std::thread stopper([&] { ctx.stop(); });
    stopper.join();
    assert(ctx.stopped());
    assert(ctx.run() == 0);
    ctx.restart();
    ctx.run();
    assert(n == 1);
  }

  // Counted work (on_work_started) keeps run() alive until on_work_finished from another thread.
  {
    io::io_context ctx;
    auto ex = ctx.get_executor();
    ex.on_work_started();
    std::atomic<int> n{0};
    std::thread t([&] {
      std::this_thread::sleep_for(ms(50));
      io::run_async(ex)(bump(n));
      std::this_thread::sleep_for(ms(50));
      ex.on_work_finished();
    });
    watchdog wd(ctx);
    ctx.run();
    t.join();
    assert(n == 1);
  }

  // Posting from other threads while run() blocks (with work counted so that it does not return early).
  {
    io::io_context ctx;
    auto ex = ctx.get_executor();
    ex.on_work_started();
    std::atomic<int> n{0};
    std::vector<std::thread> ts;
    for (int t = 0; t != 4; ++t)
      ts.emplace_back([&] {
        for (int i = 0; i != 250; ++i)
          io::run_async(ex)(bump(n));
      });
    std::thread finisher([&] {
      for (auto& t : ts)
        t.join();
      ex.on_work_finished();
    });
    watchdog wd(ctx);
    ctx.run();
    finisher.join();
    assert(n == 1000);
  }

  // Several threads in run(): the work is spread, all of it is done, and every run() returns.
  {
    io::io_context ctx;
    std::atomic<int> n{0};
    for (int i = 0; i != 200; ++i)
      io::run_async(ctx.get_executor())(hopper(n, 20));
    watchdog wd(ctx);
    std::vector<std::thread> ts;
    for (int t = 0; t != 4; ++t)
      ts.emplace_back([&] { ctx.run(); });
    for (auto& t : ts)
      t.join();
    assert(n == 200);
    assert(ctx.stopped());
  }

  // dispatch: queued from outside, inline (symmetric transfer) from inside run().
  {
    io::continuation c; // outlives the context that holds it, as required of a posted continuation
    io::io_context ctx;
    auto ex = ctx.get_executor();
    assert(!ex.running_in_this_thread());
    bool ran = false;
    struct probe {
      static io::task<void> go(io::io_context::executor_type ex, bool& ran) {
        assert(ex.running_in_this_thread());
        io::continuation c;
        c.h = std::noop_coroutine();
        auto h = ex.dispatch(c);
        assert(h == c.h); // inline
        ran = true;
        co_return;
      }
    };
    io::run_async(ex)(probe::go(ex, ran));
    ctx.run();
    assert(ran);
    // c has a null handle: never resumed; the context drops it when it is destroyed
    auto h = ex.dispatch(c); // outside the loop: posted, a no-op handle is returned
    assert(h == std::noop_coroutine());
  }

  return 0;
}
