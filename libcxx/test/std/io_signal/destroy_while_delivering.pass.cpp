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

// A signal is delivered to the sets of every context of the process, by the thread that drains the signal pipe: the
// thread that runs one context completes the waits of the others. Contexts that are destroyed while another one is
// delivering must not be touched afterwards (the completion of a wait of a context that is being destroyed). Stress
// test with SIGUSR1 and SIGUSR2 (raised with raise(), so that the handler runs in the raising thread): the main thread
// creates a context with a pending wait, raises a signal and destroys the context at once, many times, while a second
// thread runs a context that also waits for the signals.

#include <io>
#include <io_signal>
#include <io_context>

#include <atomic>
#include <cassert>
#include <chrono>
#include <coroutine>
#include <csignal>
#include <thread>
#include <utility>

#include "test_macros.h"

namespace io = std::io;

struct sentinel {
  std::atomic<int>* n;
  explicit sentinel(std::atomic<int>* p) : n(p) {}
  sentinel(sentinel&& o) noexcept : n(std::exchange(o.n, nullptr)) {}
  ~sentinel() {
    if (n)
      ++*n;
  }
};

io::task<void> wait_loop(io::signal_set* s, std::atomic<int>* delivered) {
  for (;;) {
    auto [ec, n] = co_await s->wait();
    if (ec)
      co_return;
    ++*delivered;
  }
}

io::task<void> wait_once(io::signal_set* s, sentinel) { (void)co_await s->wait(); }

int main(int, char**) {
  io::io_context ctx_a;
  io::signal_set set_a(ctx_a);
  assert(!set_a.add(SIGUSR1, io::signal_set::dont_care));
  assert(!set_a.add(SIGUSR2, io::signal_set::dont_care));
  std::atomic<int> delivered{0};
  io::run_async(ctx_a.get_executor())(wait_loop(&set_a, &delivered));
  std::thread runner([&] { ctx_a.run(); });

  const auto start = std::chrono::steady_clock::now();
  std::atomic<int> destroyed{0};
  int rounds = 0;
  while (rounds != 400 && std::chrono::steady_clock::now() - start < std::chrono::seconds(2)) {
    {
      io::io_context ctx_b;
      io::signal_set set_b(ctx_b);
      assert(!set_b.add(SIGUSR1, io::signal_set::dont_care));
      assert(!set_b.add(SIGUSR2, io::signal_set::dont_care));
      io::run_async(ctx_b.get_executor())(wait_once(&set_b, sentinel(&destroyed)));
      ctx_b.poll(); // the wait is pending
      (void)std::raise(rounds % 2 ? SIGUSR1 : SIGUSR2);
      if (rounds % 3 == 0)
        std::this_thread::yield();
      // destroyed now: the thread of ctx_a may be completing the wait of set_b
    }
    ++rounds;
  }
  // Every context that was destroyed destroyed the frame of its wait (the wait that was completed by a delivery has
  // finished normally or is destroyed with the queue of the context; either way its sentinel is gone).
  assert(destroyed == rounds);

  // The context that kept running received signals as well.
  (void)std::raise(SIGUSR1);
  auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
  while (delivered == 0 && std::chrono::steady_clock::now() < deadline)
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  assert(delivered > 0);
  ctx_a.stop(); // cancel() could land between two waits, and then the loop would go on
  runner.join();
  return 0;
}
