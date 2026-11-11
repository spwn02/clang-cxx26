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

// <io_context>

// The detachable operation of the __blocking_service (the mechanism of the resolver): a cancel() or a stop request
// completes an operation at once even while its call is in progress; the helper thread finishes the call, discards the
// result and does not touch the operation (whose frame may be gone). Also: a context that is shut down while a call is
// in progress waits for the call and drops the result.

#include <io_context>
#include <io_timer>
#include <__io_context/blocking_service.h>

#include <atomic>
#include <cassert>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <stop_token>
#include <thread>

#include "test_ctx.h"
#include "test_macros.h"

using namespace test_ctx;

struct gate {
  std::mutex m;
  std::condition_variable cv;
  bool started = false, open = false, finished = false;
  void wait_started() {
    std::unique_lock l(m);
    cv.wait(l, [&] { return started; });
  }
  void release() {
    {
      std::lock_guard l(m);
      open = true;
    }
    cv.notify_all();
  }
  void wait_finished() {
    std::unique_lock l(m);
    cv.wait(l, [&] { return finished; });
  }
  static void run(void* p) noexcept {
    gate& g = *static_cast<gate*>(p);
    std::unique_lock l(g.m);
    g.started = true;
    g.cv.notify_all();
    g.cv.wait(l, [&] { return g.open; });
    g.finished = true;
    g.cv.notify_all();
  }
};

struct op : io::__blocking_op {
  op(io::__blocking_service* s, const void* owner, std::shared_ptr<gate> g) : io::__blocking_op(s, owner) {
    __detached_work_ = &gate::run;
    __data_          = std::move(g);
  }
  op(op&&) noexcept = default;
  io::io_result<int> await_resume() noexcept {
    __finish();
    return io::io_result<int>(__ec_, 0);
  }
};

struct sentinel {
  std::atomic<int>* n;
  explicit sentinel(std::atomic<int>* p) : n(p) {}
  sentinel(sentinel&& o) noexcept : n(std::exchange(o.n, nullptr)) {}
  ~sentinel() {
    if (n)
      ++*n;
  }
};

io::task<void> call(io::io_context& ctx, const void* owner, std::shared_ptr<gate> g, std::error_code* out, bool* done) {
  op o(&ctx.use_service<io::__blocking_service>(), owner, std::move(g));
  auto [ec, n] = co_await std::move(o);
  *out         = ec;
  *done        = true;
}

io::task<void> call_pending(io::io_context& ctx, std::shared_ptr<gate> g, std::atomic<int>* d, sentinel) {
  op o(&ctx.use_service<io::__blocking_service>(), &ctx, std::move(g));
  (void)co_await std::move(o);
}

int main(int, char**) {
  // cancel of the owner while the call is in progress completes the operation at once
  {
    io::io_context ctx;
    auto g = std::make_shared<gate>();
    std::error_code ec;
    bool done = false;
    int owner = 0;
    io::run_async(ctx.get_executor())(call(ctx, &owner, g, &ec, &done));
    std::thread t([&] {
      g->wait_started();
      ctx.use_service<io::__blocking_service>().__cancel_owner(&owner);
    });
    watchdog wd(ctx);
    ctx.run(); // returns when the operation completed (while the call is still blocked)
    t.join();
    assert(done && ec == io::error::canceled);
    assert(!g->finished);
    g->release();
    g->wait_finished(); // the thread finished the call and dropped the result
  }

  // a stop request does the same
  {
    io::io_context ctx;
    auto g = std::make_shared<gate>();
    std::error_code ec;
    bool done = false;
    std::stop_source src;
    io::run_async(ctx.get_executor(), src.get_token())(call(ctx, nullptr, g, &ec, &done));
    std::thread t([&] {
      g->wait_started();
      src.request_stop();
    });
    watchdog wd(ctx);
    ctx.run();
    t.join();
    assert(done && ec == io::error::canceled && !g->finished);
    g->release();
    g->wait_finished();
  }

  // the result of a call that completes normally is delivered
  {
    io::io_context ctx;
    auto g             = std::make_shared<gate>();
    std::error_code ec = std::make_error_code(std::errc::invalid_argument);
    bool done          = false;
    io::run_async(ctx.get_executor())(call(ctx, nullptr, g, &ec, &done));
    std::thread t([&] {
      g->wait_started();
      g->release();
    });
    watchdog wd(ctx);
    ctx.run();
    t.join();
    assert(done && !ec);
  }

  // the context is shut down while the call is in progress: the destructor waits for the call (released from another
  // thread), drops the result and destroys the chain
  {
    std::atomic<int> d{0};
    auto g = std::make_shared<gate>();
    std::thread t;
    {
      io::io_context ctx;
      io::run_async(ctx.get_executor())(call_pending(ctx, g, &d, sentinel(&d)));
      ctx.poll();
      g->wait_started();
      t = std::thread([&] {
        std::this_thread::sleep_for(ms(100));
        g->release();
      });
    }
    t.join();
    assert(g->finished && d == 1);
  }
  return 0;
}
