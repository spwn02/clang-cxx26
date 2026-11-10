//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23, c++26
// UNSUPPORTED: no-threads
// REQUIRES: linux
// ADDITIONAL_COMPILE_FLAGS: -fexperimental-library

// <io_timer>

// The completion of a wait is posted to the executor of the awaiting coroutine, not to the context that drives the timer:
// a task that runs on a thread_pool and awaits a timer of an io_context resumes on a pool thread.

#include <io_timer>
#include <io_context>

#include <cassert>
#include <thread>

#include "test_ctx.h"
#include "test_macros.h"

using namespace test_ctx;

io::task<void> wait_on(io::timer& t, std::thread::id* before, std::thread::id* after, std::error_code* out) {
  *before   = std::this_thread::get_id();
  auto [ec] = co_await t.wait();
  *after    = std::this_thread::get_id();
  *out      = ec;
}

int main(int, char**) {
  io::io_context ctx;
  io::thread_pool pool(2);
  io::timer t(ctx, ms(30));
  std::thread::id before, after;
  std::error_code ec = std::make_error_code(std::errc::invalid_argument);
  std::atomic<bool> done{false};
  io::run_async(pool.get_executor(), [&] { done = true; }, [](std::exception_ptr) {})(wait_on(t, &before, &after, &ec));
  watchdog wd(ctx);
  std::thread loop([&] {
    while (!done.load() && !wd.timed_out.load()) {
      ctx.restart();
      ctx.run();
      std::this_thread::sleep_for(ms(2));
    }
  });
  while (!done.load() && !wd.timed_out.load())
    std::this_thread::sleep_for(ms(2));
  ctx.stop();
  loop.join();
  assert(done.load());
  assert(!ec);
  assert(before != std::this_thread::get_id());
  assert(after != std::this_thread::get_id());
  assert(after != loop.get_id()); // resumed on a pool thread, not on the thread that runs the io_context
  pool.join();
  return 0;
}
