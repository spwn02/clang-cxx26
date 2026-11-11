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

// <io_signal>

// signal_set::cancel() and the destruction of a signal_set with an executor whose post() resumes the coroutine at once,
// inside the call: no lock of the set, its service or the registry is held while the completion is posted, so the
// resumed coroutine can use the set again (wait once more, add or remove a signal).

#include <io_signal>
#include <io_context>

#include <atomic>
#include <cassert>
#include <coroutine>
#include <csignal>
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
  std::error_code ec;
  int signo = -1;
};

io::task<void> waiter(io::signal_set* s, result_t* r) {
  auto [ec, n] = co_await s->wait();
  r->ec        = ec;
  r->signo     = n;
  ++r->finished;
}

// Resumed inside cancel(): registers another signal on the set (takes the registry lock) and waits again.
io::task<void> reregistering_waiter(io::signal_set* s, result_t* r) {
  auto [ec, n] = co_await s->wait();
  r->ec        = ec;
  assert(!s->add(SIGUSR2));
  assert(!s->remove(SIGUSR2));
  ++r->finished;
}

int main(int, char**) {
  io::io_context ctx;
  {
    io::signal_set s(ctx, SIGUSR1);
    result_t r;
    io::run_async(inline_executor{&ctx})(waiter(&s, &r));
    assert(r.finished == 0);
    s.cancel();
    assert(r.finished == 1 && r.ec == io::error::canceled && r.signo == 0);
  }
  {
    io::signal_set s(ctx, SIGUSR1);
    result_t r;
    io::run_async(inline_executor{&ctx})(reregistering_waiter(&s, &r));
    s.cancel();
    assert(r.finished == 1 && r.ec == io::error::canceled);
  }
  {
    // The destruction of the set cancels the wait.
    result_t r;
    {
      auto s = std::make_unique<io::signal_set>(ctx, SIGUSR1);
      io::run_async(inline_executor{&ctx})(waiter(s.get(), &r));
      assert(r.finished == 0);
    }
    assert(r.finished == 1 && r.ec == io::error::canceled);
  }
  return 0;
}
