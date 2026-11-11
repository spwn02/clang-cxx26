//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23, c++26
// UNSUPPORTED: no-threads
// REQUIRES: linux || darwin || freebsd
// ADDITIONAL_COMPILE_FLAGS: -fexperimental-library

// <io_timer>

// std::io::timer: expiry, ordering of several timers, cancel/cancel_one/expires_at/expires_after (and their counts),
// expires_after(0), waiting from a task that resumes on the executor of the context, cancellation by a stop token,
// destruction of a timer with a waiter, run() staying alive while a wait is pending.

#include <io_timer>
#include <io_context>

#include <atomic>
#include <cassert>
#include <mutex>
#include <stop_token>
#include <thread>
#include <vector>

#include "test_ctx.h"
#include "test_macros.h"

using namespace test_ctx;

static bool is_canceled(std::error_code ec) { return ec == io::cond::canceled; }

io::task<void> wait_and_log(io::timer& t, int id, std::vector<int>& order, std::error_code* out = nullptr) {
  auto [ec] = co_await t.wait();
  order.push_back(id);
  if (out)
    *out = ec;
}

io::task<void> wait_ec(io::timer& t, std::error_code& out, std::thread::id* where = nullptr) {
  auto [ec] = co_await t.wait();
  out       = ec;
  if (where)
    *where = std::this_thread::get_id();
}

int main(int, char**) {
  static_assert(std::is_same_v<io::timer::clock_type, std::chrono::steady_clock>);

  // A wait completes after the delay, not before; the task resumes on the thread of the context.
  {
    io::io_context ctx;
    io::timer t(ctx, ms(30));
    std::error_code ec = std::make_error_code(std::errc::invalid_argument);
    std::thread::id where;
    auto start = clk::now();
    io::run_async(ctx.get_executor())(wait_ec(t, ec, &where));
    watchdog wd(ctx);
    ctx.run();
    auto took = clk::now() - start;
    assert(!ec);
    assert(took >= ms(30));
    assert(took < ms(2000));
    assert(where == std::this_thread::get_id());
  }

  // Several timers fire in the order of their deadlines (registered in a different order).
  {
    io::io_context ctx;
    auto now = clk::now();
    io::timer t1(ctx, now + ms(60)), t2(ctx, now + ms(20)), t3(ctx, now + ms(40)), t4(ctx, now + ms(80));
    std::vector<int> order;
    io::run_async(ctx.get_executor())(wait_and_log(t1, 1, order));
    io::run_async(ctx.get_executor())(wait_and_log(t4, 4, order));
    io::run_async(ctx.get_executor())(wait_and_log(t2, 2, order));
    io::run_async(ctx.get_executor())(wait_and_log(t3, 3, order));
    watchdog wd(ctx);
    ctx.run();
    assert((order == std::vector<int>{2, 3, 1, 4}));
  }

  // expires_after(0) and an expiry in the past: the wait completes without suspending.
  {
    io::io_context ctx;
    io::timer t(ctx);
    assert(t.expires_after(ms(0)) == 0);
    std::error_code ec = std::make_error_code(std::errc::invalid_argument);
    io::run_async(ctx.get_executor())(wait_ec(t, ec));
    watchdog wd(ctx);
    ctx.run();
    assert(!ec);
    // The default expiry is the epoch: expired.
    io::timer u(ctx);
    ec = std::make_error_code(std::errc::invalid_argument);
    ctx.restart();
    io::run_async(ctx.get_executor())(wait_ec(u, ec));
    ctx.run();
    assert(!ec);
  }

  // cancel() completes all waits with error::canceled and returns their number; cancel() with no waits returns 0.
  {
    io::io_context ctx;
    io::timer t(ctx, ms(10000));
    assert(t.cancel() == 0);
    std::vector<int> order;
    std::error_code e1, e2, e3;
    io::run_async(ctx.get_executor())(wait_and_log(t, 1, order, &e1));
    io::run_async(ctx.get_executor())(wait_and_log(t, 2, order, &e2));
    io::run_async(ctx.get_executor())(wait_and_log(t, 3, order, &e3));
    ctx.poll(); // the three tasks start and suspend
    assert(order.empty());
    assert(t.cancel() == 3);
    watchdog wd(ctx);
    ctx.restart();
    ctx.run();
    assert(is_canceled(e1) && is_canceled(e2) && is_canceled(e3));
    assert((order == std::vector<int>{1, 2, 3}));
    assert(e1 == io::error::canceled);
  }

  // cancel_one() cancels the oldest wait only.
  {
    io::io_context ctx;
    io::timer t(ctx, ms(10000));
    assert(t.cancel_one() == 0);
    std::vector<int> order;
    std::error_code e1, e2;
    io::run_async(ctx.get_executor())(wait_and_log(t, 1, order, &e1));
    io::run_async(ctx.get_executor())(wait_and_log(t, 2, order, &e2));
    ctx.poll();
    assert(t.cancel_one() == 1);
    ctx.restart();
    ctx.poll();
    assert((order == std::vector<int>{1}));
    assert(is_canceled(e1));
    assert(t.cancel_one() == 1);
    ctx.restart();
    ctx.poll();
    assert((order == std::vector<int>{1, 2}));
    assert(is_canceled(e2));
  }

  // expires_at / expires_after cancel the pending waits, return their number and move the expiry.
  {
    io::io_context ctx;
    io::timer t(ctx, ms(10000));
    std::vector<int> order;
    std::error_code e1;
    io::run_async(ctx.get_executor())(wait_and_log(t, 1, order, &e1));
    ctx.poll();
    auto at = clk::now() + ms(20);
    assert(t.expires_at(at) == 1);
    assert(t.expiry() == at);
    assert(t.expires_after(ms(20)) == 0);
    ctx.restart();
    ctx.poll();
    assert(is_canceled(e1));
    // A new wait now fires after the new expiry.
    std::error_code e2 = std::make_error_code(std::errc::invalid_argument);
    io::run_async(ctx.get_executor())(wait_ec(t, e2));
    ctx.restart();
    watchdog wd(ctx);
    ctx.run();
    assert(!e2);
  }

  // Cancellation by the stop token of the awaiting chain, while the wait is pending.
  {
    io::io_context ctx;
    io::timer t(ctx, ms(10000));
    std::stop_source src;
    std::error_code ec;
    io::run_async(ctx.get_executor(), src.get_token())(wait_ec(t, ec));
    std::thread requester([&] {
      std::this_thread::sleep_for(ms(30));
      src.request_stop();
    });
    auto start = clk::now();
    watchdog wd(ctx);
    ctx.run();
    requester.join();
    assert(is_canceled(ec));
    assert(clk::now() - start < ms(2000));
  }

  // A stop that was requested before the wait: it completes with canceled without suspending.
  {
    io::io_context ctx;
    io::timer t(ctx, ms(10000));
    std::stop_source src;
    src.request_stop();
    std::error_code ec;
    io::run_async(ctx.get_executor(), src.get_token())(wait_ec(t, ec));
    watchdog wd(ctx);
    ctx.run();
    assert(is_canceled(ec));
  }

  // A pending wait keeps run() alive; destroying the timer completes it with canceled.
  {
    io::io_context ctx;
    auto t = std::make_unique<io::timer>(ctx, ms(10000));
    std::error_code ec;
    io::run_async(ctx.get_executor())(wait_ec(*t, ec));
    std::thread killer([&] {
      std::this_thread::sleep_for(ms(40));
      t->cancel();
    });
    watchdog wd(ctx);
    ctx.run();
    killer.join();
    assert(is_canceled(ec));
  }

  // Waits on several timers from different threads running the context.
  {
    io::io_context ctx;
    std::vector<std::unique_ptr<io::timer>> ts;
    std::vector<std::error_code> ecs(8);
    for (int i = 0; i != 8; ++i) {
      ts.push_back(std::make_unique<io::timer>(ctx, ms(10 + 5 * i)));
      ecs[i] = std::make_error_code(std::errc::invalid_argument);
      io::run_async(ctx.get_executor())(wait_ec(*ts[i], ecs[i]));
    }
    watchdog wd(ctx);
    std::thread a([&] { ctx.run(); }), b([&] { ctx.run(); });
    a.join();
    b.join();
    for (auto& ec : ecs)
      assert(!ec);
  }

  // Executor-based construction.
  {
    io::io_context ctx;
    io::timer t(ctx.get_executor(), ms(1));
    std::error_code ec = std::make_error_code(std::errc::invalid_argument);
    io::run_async(ctx.get_executor())(wait_ec(t, ec));
    watchdog wd(ctx);
    ctx.run();
    assert(!ec);
  }

  return 0;
}
