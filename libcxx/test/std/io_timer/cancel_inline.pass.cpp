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

// cancel(), cancel_one(), expires_after() and the destruction of a timer with an executor whose post() resumes the
// coroutine at once, inside the call: the cancellation holds no lock of the timer or the reactor while it posts, so the
// resumed coroutine can use the timer again (start another wait, cancel it).

#include <io_timer>
#include <io_context>

#include <atomic>
#include <cassert>
#include <coroutine>
#include <memory>

#include "test_ctx.h"
#include "test_macros.h"

using namespace test_ctx;

struct inline_executor {
  io::io_context* c;
  bool operator==(const inline_executor&) const noexcept { return true; }
  io::io_context& context() const noexcept { return *c; }
  void on_work_started() const noexcept {}
  void on_work_finished() const noexcept {}
  std::coroutine_handle<> dispatch(io::continuation& k) const { return k.h; }
  void post(io::continuation& k) const { k.h.resume(); }
};

struct result_t {
  std::atomic<int> finished{0};
  std::error_code ec[4];
};

io::task<void> waiter(io::timer* t, result_t* r, int i) {
  auto [ec] = co_await t->wait();
  r->ec[i]  = ec;
  ++r->finished;
}

// Resumed inside the cancellation: uses the timer again (cancels it) before it finishes.
io::task<void> rewaiter(io::timer* t, result_t* r, int i) {
  auto [ec] = co_await t->wait();
  r->ec[i]  = ec;
  t->cancel();
  ++r->finished;
}

int main(int, char**) {
  io::io_context ctx;
  {
    io::timer t(ctx, std::chrono::hours(1));
    result_t r;
    for (int i = 0; i != 3; ++i)
      io::run_async(inline_executor{&ctx})(waiter(&t, &r, i));
    assert(r.finished == 0);
    assert(t.cancel_one() == 1);
    assert(r.finished == 1 && r.ec[0] == io::error::canceled);
    assert(t.cancel() == 2);
    assert(r.finished == 3 && r.ec[1] == io::error::canceled && r.ec[2] == io::error::canceled);
  }
  {
    io::timer t(ctx, std::chrono::hours(1));
    result_t r;
    io::run_async(inline_executor{&ctx})(rewaiter(&t, &r, 0));
    io::run_async(inline_executor{&ctx})(rewaiter(&t, &r, 1));
    assert(t.expires_after(std::chrono::hours(2)) == 2);
    assert(r.finished == 2 && r.ec[0] == io::error::canceled && r.ec[1] == io::error::canceled);
  }
  {
    // The destruction of the timer cancels the wait; the coroutine only records the result.
    result_t r;
    {
      auto t = std::make_unique<io::timer>(ctx, std::chrono::hours(1));
      io::run_async(inline_executor{&ctx})(waiter(t.get(), &r, 0));
      assert(r.finished == 0);
    }
    assert(r.finished == 1 && r.ec[0] == io::error::canceled);
  }
  return 0;
}
