//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef TEST_STD_IO_CONTEXT_TEST_CTX_H
#define TEST_STD_IO_CONTEXT_TEST_CTX_H

#include <atomic>
#include <cassert>
#include <chrono>
#include <coroutine>
#include <io>
#include <io_context>
#include <system_error>
#include <thread>

namespace test_ctx {

namespace io = std::io;
using clk    = std::chrono::steady_clock;
using ms     = std::chrono::milliseconds;

// A single-shot, thread-safe flag for the tests that wait for a coroutine from the outside.
struct done_flag {
  std::atomic<bool> v{false};
  void set() { v.store(true); }
  bool is_set() const { return v.load(); }
};

// Runs ctx on the calling thread, with a watchdog that stops it if something hangs (the test then fails its asserts).
struct watchdog {
  io::io_context& ctx;
  std::atomic<bool> finished{false};
  std::thread t;
  explicit watchdog(io::io_context& c, ms limit = ms(8000))
      : ctx(c), t([this, limit] {
          auto end = clk::now() + limit;
          while (!finished.load() && clk::now() < end)
            std::this_thread::sleep_for(ms(5));
          if (!finished.load()) {
            ctx.stop();
            timed_out.store(true);
          }
        }) {}
  std::atomic<bool> timed_out{false};
  ~watchdog() {
    finished.store(true);
    t.join();
    assert(!timed_out.load() && "the test hung");
  }
};

} // namespace test_ctx

#endif // TEST_STD_IO_CONTEXT_TEST_CTX_H
