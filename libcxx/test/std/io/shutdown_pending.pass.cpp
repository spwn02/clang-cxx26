//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23, c++26
// UNSUPPORTED: no-threads
// ADDITIONAL_COMPILE_FLAGS: -fexperimental-library

// <io>

// A thread_pool (or any execution_context) that is destroyed while chains of coroutines are suspended or queued in it
// destroys every frame of the chains, also when the chains go through a strand that outlives the pool, and also when
// the pool is destroyed while its workers are running chains.

#include <io>

#include <atomic>
#include <cassert>
#include <chrono>
#include <coroutine>
#include <memory>
#include <optional>
#include <thread>
#include <utility>
#include <vector>

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

// An awaitable that never completes.
struct never {
  bool await_ready() const noexcept { return false; }
  void await_suspend(std::coroutine_handle<>, const io::io_env*) noexcept {}
  void await_resume() const noexcept {}
};

// The sentinel is a parameter, so it lives in the frame from the moment the task is created, also if the body never ran.
io::task<void> inner(std::atomic<int>* d, sentinel) { co_await never{}; }
io::task<void> middle(std::atomic<int>* d, sentinel) { co_await inner(d, sentinel(d)); }
io::task<void> outer(std::atomic<int>* d, sentinel) { co_await middle(d, sentinel(d)); }
io::task<void> quick(std::atomic<int>* d, sentinel) { co_return; }
io::task<void> joined(std::atomic<int>* d, sentinel) {
  co_await io::when_all(inner(d, sentinel(d)), inner(d, sentinel(d)));
}

int main(int, char**) {
  // Queued roots of a pool that was stopped before they ran.
  {
    std::atomic<int> d{0};
    {
      io::thread_pool pool(2);
      pool.stop();
      for (int i = 0; i != 10; ++i)
        io::run_async(pool.get_executor())(outer(&d, sentinel(&d)));
    }
    assert(d == 10);
  }

  // Chains suspended in the pool, three deep, and in a when_all.
  {
    std::atomic<int> d{0};
    {
      io::thread_pool pool(2);
      for (int i = 0; i != 10; ++i)
        io::run_async(pool.get_executor())(outer(&d, sentinel(&d)));
      io::run_async(pool.get_executor())(joined(&d, sentinel(&d)));
      std::this_thread::sleep_for(std::chrono::milliseconds(100)); // lets the workers start them
      assert(d == 0);
    }
    assert(d == 10 * 3 + 1 + 2);
  }

  // Roots on a strand that outlives the pool: the strand state must not walk through freed frames.
  {
    std::atomic<int> d{0};
    std::optional<io::thread_pool> pool;
    pool.emplace(1);
    pool->stop(); // nothing runs: the roots wait in the queue of the strand, the drain in the queue of the pool
    io::strand s(pool->get_executor());
    io::strand s2 = s;
    (void)s2;
    for (int i = 0; i != 5; ++i)
      io::run_async(s)(outer(&d, sentinel(&d)));
    pool.reset();
    assert(d == 5); // the roots (never started) are destroyed once; the strand copies are destroyed at the end
  }

  // Strand roots that are suspended, the strand outliving the pool.
  {
    std::atomic<int> d{0};
    std::optional<io::thread_pool> pool;
    pool.emplace(2);
    io::strand s(pool->get_executor());
    for (int i = 0; i != 5; ++i)
      io::run_async(s)(outer(&d, sentinel(&d)));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    pool.reset();
    assert(d == 15);
  }

  // Destruction while the workers are running chains: every frame is destroyed or finished, exactly once. Several
  // threads launch concurrently.
  for (int round = 0; round != 20; ++round) {
    std::atomic<int> d{0};
    constexpr int per_thread = 50;
    constexpr int nthreads   = 3;
    {
      io::thread_pool pool(4);
      std::vector<std::thread> ts;
      for (int t = 0; t != nthreads; ++t)
        ts.emplace_back([&] {
          for (int i = 0; i != per_thread; ++i) {
            if (i % 2)
              io::run_async(pool.get_executor())(quick(&d, sentinel(&d)));
            else
              io::run_async(pool.get_executor())(inner(&d, sentinel(&d)));
          }
        });
      for (std::thread& t : ts)
        t.join();
    }
    assert(d == nthreads * per_thread);
  }

  return 0;
}
